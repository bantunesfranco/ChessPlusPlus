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

    Score negamax(Board& board, Depth depth, Score alpha, Score beta);
    Score quiescence(Board& board, Score alpha, Score beta);
    Score evaluate(const Board& board);

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

// ============================================================================
// Quiescence Search - Handle Tactical Positions
// ============================================================================

Score Engine::Impl::quiescence(Board& board, Score alpha, const Score beta) {
    if (stop_requested) return 0;

    stats.nodes++;

    // Check terminal states
    if (board.is_checkmate())
        return board.side_to_move() == Color::WHITE ? -CHECKMATE : CHECKMATE;

    if (board.is_stalemate())
        return STALEMATE;

    // Stand-pat: position value without any moves
    const Score stand_pat = evaluate(board);

    if (stand_pat >= beta)
        return beta;  // Pruning

    if (alpha < stand_pat)
        alpha = stand_pat;

    // Only generate capture moves in quiescence
    MoveList captures;
    board.generate_captures(captures);

    // Try each capture
    for (const auto capture : captures) {
        board.make_move(capture);

        const Score score = -quiescence(board, -beta, -alpha);
        board.undo_move();

        if (score >= beta) {
            return beta;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    return alpha;
}

// ============================================================================
// Main Search - Negamax with Alpha-Beta Pruning
// ============================================================================

Score Engine::Impl::negamax(Board& board, Depth depth, Score alpha, const Score beta) {
    if (stop_requested) return 0;

    // Transposition table lookup
    const auto tt_entry = ttable.lookup(board.zobrist_hash(), depth);
    if (tt_entry && tt_entry->depth >= depth) {
        stats.tt_hits++;
        return tt_entry->score;
    }

    stats.nodes++;

    // Terminal states
    if (board.is_checkmate())
        return -CHECKMATE;

    if (board.is_stalemate() || board.is_50_move_draw())
        return 0;

    // Depth limit - enter quiescence search
    if (depth == 0) {
        if (config.use_quiescence_search)
            return quiescence(board, alpha, beta);
        return evaluate(board);
    }

    // Generate moves
    MoveList moves;
    board.generate_moves(moves);

    assert(!moves.empty());  // Should have been caught by terminal state checks

    // Move ordering
    Move ttmove = tt_entry ? tt_entry->best_move : Move();
    if (config.use_move_ordering)
        order_moves(moves, board, ttmove);

    Score best_score = std::numeric_limits<Score>::max();
    Move best_move = moves[0];
    int moves_searched = 0;
    Flag flag = EXACT;

    // Try each move
    for (const auto& move : moves) {
        board.make_move(move);

        // Recursively search
        const Score score = -negamax(board, depth - 1, -beta, -alpha);

        board.undo_move();

        moves_searched++;

        if (score > best_score) {
            best_score = score;
            best_move = move;
            flag = EXACT;
        }

        if (best_score > alpha)
        {
            alpha = best_score;
            flag = LOWER_BOUND;
        }

        // Beta cutoff
        if (alpha >= beta) {
            stats.cutoffs++;
                flag = LOWER_BOUND;

            // Update killer move
            if (!move.is_capture())
                killers.store(depth, move);

            break;
        }
    }

    // Update history for quiet moves
    if (!best_move.is_capture() && moves_searched > 0)
        history.store(best_move.from(), best_move.to(), depth);

    // Store in transposition table
    if (config.use_transposition_table)
        ttable.store(board.zobrist_hash(), best_score, depth, flag, best_move);

    return best_score;
}

// ============================================================================
// Iterative Deepening - Progressive Deepening with Time Management
// ============================================================================

SearchResult Engine::Impl::search_iterative(Board& board, const std::chrono::milliseconds time_limit) {
    stats.start_time = std::chrono::high_resolution_clock::now();
    stop_requested = false;

    SearchResult best_result;
    best_result.best_move = Move();
    best_result.score = 0;
    best_result.depth = 0;
    best_result.nodes_searched = 0;

    // Iterative deepening: search depth 1, 2, 3, ... until time runs out
    for (int depth = 1; depth <= config.max_depth; ++depth) {
        history.clear();
        killers.clear();

        // Alpha-beta window
        Score alpha = -50000;
        Score beta = 50000;

        // Root search at this depth
        MoveList moves;
        board.generate_moves(moves);

        if (moves.empty()) {
            break;  // No legal moves
        }

        Score best_score = INT_MIN;
        Move best_move = moves[0];

        for (const auto& move : moves) {
            board.make_move(move);
            const Score score = -negamax(board, depth - 1, -beta, -alpha);
            board.undo_move();

            if (score > best_score) {
                best_score = score;
                best_move = move;
                alpha = best_score;
            }
        }

        // Update result
        best_result.best_move = best_move;
        best_result.score = best_score;
        best_result.depth = depth;
        best_result.nodes_searched = stats.nodes;
        best_result.search_time = stats.elapsed_seconds();

        // Report iteration
        std::cout << "Depth " << depth << ": "
                 << square_to_string(best_move.from()) << "-" << square_to_string(best_move.to())
                 << " (score: " << best_score << ", nodes: " << stats.nodes
                 << ", time: " << best_result.search_time << "s)" << std::endl;

        if (config.on_iteration_complete)
            config.on_iteration_complete(best_result);

        // Check time limit
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

    SearchResult result = {};

    MoveList moves;
    board.generate_moves(moves);

    if (moves.empty()) {
        return result;  // No legal moves
    }

    Score best_score = INT_MIN;
    Move best_move = moves[0];

    for (const auto& move : moves) {
        board.make_move(move);
        Score score = -negamax(board, max_depth - 1, (Score)-50000, (Score)50000);
        board.undo_move();

        if (score > best_score) {
            best_score = score;
            best_move = move;
        }
    }

    result.best_move = best_move;
    result.score = best_score;
    result.depth = max_depth;
    result.nodes_searched = stats.nodes;
    result.search_time = stats.elapsed_seconds();

    return result;
}

Score Engine::Impl::evaluate(const Board& board) {
    // Negate score for negamax
    return evaluator.evaluate(board);
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