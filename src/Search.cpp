#include "chess/Search.hpp"
#include "chess/ZobristHasher.hpp"
#include "chess/TranspositionTable.hpp"
#include "chess/Eval.hpp"
#include <algorithm>
#include <cstring>
#include <iostream>
#include <ranges>

#include "chess/PieceSquareTables.hpp"
#include "chess/internal/Bitboard.hpp"

namespace chess {

constexpr Score MATE_THRESHOLD = CHECKMATE - 1000;

inline Score score_to_tt(Score score, Depth depth) {
    if (score > MATE_THRESHOLD)
        return score + depth;     // mate for us
    if (score < -MATE_THRESHOLD)
        return score - depth;     // mate for opponent
    return score;
}

inline Score score_from_tt(Score score, Depth depth) {
    if (score > MATE_THRESHOLD)
        return score - depth;
    if (score < -MATE_THRESHOLD)
        return score + depth;
    return score;
}


// ============================================================================
// Internal Search Helpers (Private to Search.cpp)
// ============================================================================

class HistoryHeuristic {
private:
    std::array<std::array<int, 64>, 64> history{};

public:
    void store(const Square from, const Square to, const Depth d) {
        history[(int)from][(int)to] += d * d;
    }

    [[nodiscard]] int get_score(const Square from, const Square to) const {
        return history[(int)from][(int)to];
    }

    void clear() {
        for (auto& row : history) row.fill(0);
    }
};

class KillerMoves {
private:
    static constexpr int MAX_DEPTH = 32;
    std::array<std::array<Move, 2>, MAX_DEPTH> killers;

public:
    KillerMoves() { clear(); }

    void store(const Depth d, const Move m) {
        if (d < MAX_DEPTH && killers[d][0] != m) {
            killers[d][1] = killers[d][0];
            killers[d][0] = m;
        }
    }

    [[nodiscard]] bool is_killer(const Depth d, const Move m) const {
        if (d >= MAX_DEPTH) return false;
        return killers[d][0] == m || killers[d][1] == m;
    }

    void clear() {
        for (auto& row : killers) row.fill(Move());
    }
};

struct SearchStats {
    uint64_t nodes = 0;
    uint64_t tt_hits = 0;
    uint64_t cutoffs = 0;
    std::chrono::high_resolution_clock::time_point start_time;

    [[nodiscard]] double elapsed_seconds() const {
        const auto now = std::chrono::high_resolution_clock::now();
        return std::chrono::duration<double>(now - start_time).count();
    }
};

// ============================================================================
// Engine::Impl
// ============================================================================

class Engine::Impl {
public:
    SearchConfig config;
    HistoryHeuristic history;
    KillerMoves killers;
    TranspositionTable ttable;
    PieceSquareTables pst;
    Evaluator evaluator;

    SearchStats stats{};
    bool stop_requested = false;

    Impl();
    explicit Impl(const SearchConfig& cfg);

    SearchResult search_iterative(Board& board, std::chrono::milliseconds time_limit);
    SearchResult search_fixed_depth(Board& board, Depth max_depth);

    Score negamax(Board& board, Depth depth, Depth ply, Score alpha, Score beta);
    Score quiescence(Board& board, Score alpha, Score beta);
    [[nodiscard]] Score evaluate(const Board& board) const;

    void order_moves(MoveList& moves, const Board& board, Move ttmove) const;

private:
    Move best_move_at_depth = Move();

    [[nodiscard]] int move_score(const Move& move, const Board& board, Move ttmove, Depth depth) const;
};

// ============================================================================
// Engine::Impl Implementation
// ============================================================================

Engine::Impl::Impl() : ttable(config.tt_size_mb), evaluator(pst) {}

Engine::Impl::Impl(const SearchConfig& cfg) : config(cfg) , ttable(cfg.tt_size_mb), evaluator(pst) {}

// ============================================================================
// Move Ordering - Critical for Alpha-Beta Efficiency
// ============================================================================

void Engine::Impl::order_moves(MoveList& moves, const Board& board, const Move ttmove) const {
    // Score moves for ordering
    std::vector<std::pair<int, Move>> scored;
    scored.reserve(moves.size());

    for (auto& move : moves) {
        int score = move_score(move, board, ttmove, 0);
        scored.emplace_back(score, move);
    }

    // Sort by score (descending)
    std::ranges::sort(scored,
                      [](const auto& a, const auto& b) { return a.first > b.first; });

    // Rebuild moves list in sorted order
    moves.clear();
    for (const auto& move : scored | std::views::values) {
        moves.add(move);
    }
}

int Engine::Impl::move_score(const Move& move, const Board& board, const Move ttmove, const Depth depth) const {
    // Transposition table move - highest priority
    if (move == ttmove) return 1000000;

    // Captures - MVV/LVA scoring
    if (move.flag() == MoveFlag::CAPTURE) {
        const Piece victim = board.piece_at(move.to());
        const Piece attacker = board.piece_at(move.from());

        const int victim_value = PIECE_VALUES[(int)get_piece_type(victim)];
        const int attacker_value = PIECE_VALUES[(int)get_piece_type(attacker)];

        return 500000 + (victim_value * 10 - attacker_value);
    }

    // Killer moves
    if (killers.is_killer(depth, move)) return 90000;

    // Quiet moves - history heuristic
    return history.get_score(move.from(), move.to());
}

Score Engine::Impl::quiescence(Board& board, Score alpha, Score beta) {
    if (stop_requested)
        return 0;

    stats.nodes++;

    if (board.is_checkmate())
        return -CHECKMATE;
    if (board.is_stalemate())
        return 0;

    Score stand_pat = evaluate(board);

    if (stand_pat >= beta)
        return beta;
    if (stand_pat > alpha)
        alpha = stand_pat;

    MoveList moves;
    // Include captures and checks in quiescence
    board.generate_captures(moves);
    board.generate_checks(moves);

    // MVV/LVA ordering
    std::ranges::sort(moves, [&](const Move& a, const Move& b) {
        const Piece victimA = board.piece_at(a.to());
        const Piece victimB = board.piece_at(b.to());
        const Piece attackerA = board.piece_at(a.from());
        const Piece attackerB = board.piece_at(b.from());
        int scoreA = PIECE_VALUES[(int)get_piece_type(victimA)] * 10 - PIECE_VALUES[(int)get_piece_type(attackerA)];
        int scoreB = PIECE_VALUES[(int)get_piece_type(victimB)] * 10 - PIECE_VALUES[(int)get_piece_type(attackerB)];
        return scoreA > scoreB;
    });

    for (const auto& move : moves) {
        board.make_move(move);
        Score score = -quiescence(board, -beta, -alpha);
        board.undo_move();

        if (score >= beta)
            return beta;
        if (score > alpha)
            alpha = score;
    }

    return alpha;
}

Score Engine::Impl::negamax(Board& board, Depth depth, const Depth ply, Score alpha, Score beta) {
    if (stop_requested)
        return 0;

    const Score original_alpha = alpha;

    // Transposition table lookup
    Move ttmove = INVALID_MOVE;
    if (config.use_transposition_table) {
        if (auto entry = ttable.lookup(board.zobrist_hash(), depth)) {
            ttmove = entry->best_move;
            Score tt_score = score_from_tt(entry->score, depth);

            if (entry->flag == EXACT)
                return tt_score;
            else if (entry->flag == LOWER_BOUND)
                alpha = std::max(alpha, tt_score);
            else if (entry->flag == UPPER_BOUND)
                beta = std::min(beta, tt_score);

            if (alpha >= beta)
                return tt_score;
        }
    }

    stats.nodes++;

    if (board.is_checkmate())
        return -CHECKMATE + ply;
    if (board.is_stalemate() || board.is_50_move_draw())
        return 0;

    // Check extension: +1 ply if in check
    if (board.is_in_check())
        depth += 1;

    if (depth <= 0)
        return quiescence(board, alpha, beta);

    MoveList moves;
    board.generate_moves(moves);
    if (moves.empty())
        return board.is_in_check() ? -CHECKMATE + ply : 0;

    // Move ordering
    if (config.use_move_ordering)
        order_moves(moves, board, ttmove);

    Score best_score = -50000;
    Move best_move = INVALID_MOVE;
    bool first_move = true;

    for (const auto& move : moves) {
        board.make_move(move);

        int reduction = 0;
        // LMR: reduce depth for non-captures, non-first moves
        if (!first_move && depth >= 3 && !move.is_capture() && !board.is_in_check()) {
            reduction = (depth >= 6) ? 2 : 1;
        }

        Score score;
        if (first_move) {
            score = -negamax(board, depth - 1, ply + 1, -beta, -alpha);
            first_move = false;
        } else {
            // PVS / null-window
            score = -negamax(board, depth - 1 - reduction, ply + 1, -alpha - 1, -alpha);
            if (score > alpha)
                score = -negamax(board, depth - 1, ply + 1, -beta, -alpha);
        }

        board.undo_move();

        if (score > best_score) {
            best_score = score;
            best_move = move;
        }

        alpha = std::max(alpha, score);

        if (alpha >= beta) {
            stats.cutoffs++;
            if (!move.is_capture())
                killers.store(config.max_depth - depth, move);
            break;
        }
    }

    if (best_move != INVALID_MOVE && !best_move.is_capture())
        history.store(best_move.from(), best_move.to(), depth);

    // TT flag determination
    Flag flag;
    if (best_score <= original_alpha)
        flag = UPPER_BOUND;
    else if (best_score >= beta)
        flag = LOWER_BOUND;
    else
        flag = EXACT;

    if (config.use_transposition_table) {
        const Score stored_score = score_to_tt(best_score, depth);
        ttable.store(board.zobrist_hash(), stored_score, depth, flag, best_move);
    }

    return best_score;
}

// ============================================================================
// Iterative Deepening - Progressive Deepening with Time Management
// ============================================================================

SearchResult Engine::Impl::search_iterative(Board& board, const std::chrono::milliseconds time_limit) {
    stats.start_time = std::chrono::high_resolution_clock::now();
    stop_requested = false;
    stats.nodes = 0;

    SearchResult best_result{};

    for (int depth = 1; depth <= config.max_depth; ++depth) {
        MoveList moves;
        board.generate_moves(moves);

        if (moves.empty())
            break;

        Score alpha = -50000;
        Score beta = 50000;

        Score best_score = -50000;
        Move best_move = INVALID_MOVE;

        // Root TT move
        Move ttmove = INVALID_MOVE;
        if (config.use_transposition_table) {
            if (auto entry = ttable.lookup(board.zobrist_hash(), 0))
                ttmove = entry->best_move;
        }

        if (config.use_move_ordering)
            order_moves(moves, board, ttmove);

        bool first_move = true;
        int move_count = 0;

        for (const auto& move : moves) {
            move_count++;
            board.make_move(move);
            Score score;

            int reduction = 0;

            // Apply LMR at root: skip TT move and first move
            if (!first_move && move_count > 2 && depth >= 3 && !move.is_capture()) {
                reduction = (depth >= 6) ? 2 : 1;
            }

            if (first_move) {
                score = -negamax(board, depth - 1, 1, -beta, -alpha);
                first_move = false;
            } else {
                // Reduced-depth null-window search
                score = -negamax(board, depth - 1 - reduction, 1, -alpha - 1, -alpha);

                // If null-window fails high, re-search with full depth/window
                if (score > alpha)
                    score = -negamax(board, depth - 1, 1, -beta, -alpha);
            }

            board.undo_move();

            if (score > best_score) {
                best_score = score;
                best_move = move;
            }

            alpha = std::max(alpha, score);

            if (alpha >= beta)
                break;
        }

        if (best_move != INVALID_MOVE)
            history.store(best_move.from(), best_move.to(), depth);

        best_result.best_move = best_move;
        best_result.score = best_score;
        best_result.depth = depth;
        best_result.nodes_searched = stats.nodes;
        best_result.search_time = stats.elapsed_seconds();

        if (config.on_iteration_complete)
            config.on_iteration_complete(best_result);

        // Stop if time exceeded
        if (stats.elapsed_seconds() * 1000 > time_limit.count())
            break;
    }

    return best_result;
}


// ============================================================================
// Fixed Depth Search
// ============================================================================

SearchResult Engine::Impl::search_fixed_depth(Board& board, const Depth max_depth) {
    stats.start_time = std::chrono::high_resolution_clock::now();
    stop_requested = false;

    SearchResult result{};
    MoveList moves;
    board.generate_moves(moves);

    if (moves.empty())
        return result;  // No legal moves

    Score best_score = -50000;
    Move best_move = INVALID_MOVE;

    // Root move ordering
    Move ttmove = INVALID_MOVE;
    if (config.use_transposition_table) {
        if (auto entry = ttable.lookup(board.zobrist_hash(), 0))
            ttmove = entry->best_move;
    }

    if (config.use_move_ordering)
        order_moves(moves, board, ttmove);

    bool first_move = true;

    for (const auto& move : moves) {
        board.make_move(move);
        Score score;

        if (first_move) {
            score = -negamax(board, max_depth - 1, 1, -50000, 50000);
            first_move = false;
        } else {
            score = -negamax(board, max_depth - 1, 1, -best_score - 1, -best_score);
            if (score > best_score)
                score = -negamax(board, max_depth - 1, 1, -50000, 50000);
        }

        board.undo_move();

        if (score > best_score) {
            best_score = score;
            best_move = move;
        }
    }

    // Store history heuristic for best root move
    if (best_move != INVALID_MOVE && !best_move.is_capture())
        history.store(best_move.from(), best_move.to(), max_depth);

    result.best_move = best_move;
    result.score = best_score;
    result.depth = max_depth;
    result.nodes_searched = stats.nodes;
    result.search_time = stats.elapsed_seconds();

    return result;
}

Score Engine::Impl::evaluate(const Board& board) const {
    return evaluator.evaluate(board);;
}

// ============================================================================
// Engine Implementation
// ============================================================================

Engine::Engine() : impl(std::make_unique<Impl>()) {}

Engine::Engine(const SearchConfig& config) : impl(std::make_unique<Impl>(config)) {}

Engine::~Engine() = default;

void Engine::set_config(const SearchConfig& config) const {
    impl->config = config;
}

SearchConfig Engine::get_config() const {
    return impl->config;
}

void Engine::set_tt_size(const int mb) const {
    impl->config.tt_size_mb = mb;
    impl->ttable.resize(mb);
}

void Engine::clear_cache() const {
    impl->ttable.clear();
}

Score Engine::evaluate(const Board& board) const{
    return impl->evaluate(board);
}

void Engine::stop_search() const{
    impl->stop_requested = true;
}

SearchResult Engine::find_best_move(Board board, const std::chrono::milliseconds time_limit) const {
    return impl->search_iterative(board, time_limit);
}

SearchResult Engine::find_best_move(Board board, const Depth max_depth) const {
    return impl->search_fixed_depth(board, max_depth);
}

SearchResult Engine::find_best_move(Board board, const int max_depth, const std::chrono::milliseconds time_limit) const {
    impl->config.max_depth = max_depth;
    return impl->search_iterative(board, time_limit);
}

std::vector<Move> Engine::get_principal_variation(Board board, const int depth) const {
    // Extract PV from transposition table
    std::vector<Move> pv;

    for (int i = 0; i < depth; ++i) {

        const auto entry = impl->ttable.lookup(board.zobrist_hash(), 0);
        if (!entry)
            break;

        Move best_move = entry->best_move;

        pv.push_back(best_move);
        board.make_move(best_move);
    }

    return pv;
}

MoveList Engine::get_ranked_moves(const Board& board) const {
    MoveList moves;
    board.generate_moves(moves);

    impl->order_moves(moves, board, Move());

    return moves;
}

Engine::Analysis Engine::analyze(const Board& board, const int depth) const {
    const auto result = find_best_move(board, depth);

    Analysis analysis;
    analysis.best_move = result.best_move;
    analysis.score = result.score;
    analysis.depth = result.depth;
    analysis.pv = get_principal_variation(board, depth);

    return analysis;
}

void Engine::set_progress_callback(const std::function<void(int depth, uint64_t nodes)>& callback) {
    // Store callback for iterative deepening
    impl->config.on_iteration_complete = [callback](const SearchResult& result) {
        callback(result.depth, result.nodes_searched);
    };
}

}  // namespace chess