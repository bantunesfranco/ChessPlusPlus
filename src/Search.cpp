#include "chess/Search.hpp"
#include "chess/ZobristHasher.hpp"
#include "chess/TranspositionTable.hpp"
#include "chess/Eval.hpp"
#include <algorithm>
#include <cstring>

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
    void store(Square from, Square to, Depth d) {
        history[(int)from][(int)to] += d * d;
    }

    [[nodiscard]] int get_score(Square from, Square to) const {
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
        if (d < MAX_DEPTH && killers[(int)d][0] != m) {
            killers[(int)d][1] = killers[(int)d][0];
            killers[(int)d][0] = m;
        }
    }

    [[nodiscard]] bool is_killer(const Depth d, const Move m) const {
        if (d >= MAX_DEPTH) return false;
        return killers[(int)d][0] == m || killers[(int)d][1] == m;
    }

    void clear() {
        for (auto& row : killers) row.fill(Move());
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
    PieceSquareTables pieceSquareTables;
    Evaluator evaluator;
    bool stop_requested = false;

    Impl();
    explicit Impl(const SearchConfig& cfg);

    Score search(const Board& board, Depth depth, Score alpha, Score beta);
    Score quiescence(const Board& board, Score alpha, Score beta);
    Score evaluate(const Board& board);
};

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

void Engine::clear_cache() {
    impl->ttable.clear();
}

SearchResult Engine::find_best_move(const Board& board, const std::chrono::milliseconds time_limit) {
    SearchConfig& cfg = impl->config;
    cfg.time_limit = time_limit;
    (void)board;
    // TODO: Implement iterative deepening with time management
    return SearchResult{};
}

SearchResult Engine::find_best_move_depth(const Board& board, const Depth max_depth) {
    SearchConfig& cfg = impl->config;
    cfg.max_depth = max_depth;
    (void)board;
    // TODO: Implement fixed-depth search
    return SearchResult{};
}

SearchResult Engine::find_best_move(const Board& board, const int max_depth, const std::chrono::milliseconds time_limit) {
    SearchConfig& cfg = impl->config;
    cfg.max_depth = max_depth;
    cfg.time_limit = time_limit;
    (void)board;
    // TODO: Implement search with both limits
    return SearchResult{};
}

Score Engine::evaluate(const Board& board) {
    return impl->evaluate(board);
}

std::vector<Move> Engine::get_principal_variation(const Board& board, int depth) {
    // TODO: Implement PV extraction
    (void)board;
    (void)depth;
    return {};
}

MoveList Engine::get_ranked_moves(const Board& board) {
    // TODO: Implement move ranking
    MoveList moves = MoveList();
    board.generate_captures(moves);
    return moves;
}

Engine::Analysis Engine::analyze(const Board& board, int depth) {
    // TODO: Implement detailed analysis
    (void)board;
    (void)depth;
    return {};
}

void Engine::set_progress_callback(const std::function<void(int depth, uint64_t nodes)>& callback) {
    // TODO: Implement callback registration
    callback(0, 0);
}

void Engine::stop_search() {
    // TODO: Implement search stopping
}

// ============================================================================
// Engine::Impl Implementation
// ============================================================================

Engine::Impl::Impl() : ttable(config.tt_size_mb), evaluator(pieceSquareTables) {}

Engine::Impl::Impl(const SearchConfig& cfg) : config(cfg) , ttable(cfg.tt_size_mb),evaluator(pieceSquareTables) {}

Score Engine::Impl::evaluate(const Board& board) {
    return evaluator.evaluate(board);
}

Score Engine::Impl::search(const Board& board, Depth depth, Score alpha, Score beta) {
    // TODO: Implement minimax with alpha-beta pruning
    (void)board;
    (void)depth;
    (void)alpha;
    (void)beta;
    return 0;
}

Score Engine::Impl::quiescence(const Board& board, Score alpha, Score beta) {
    // TODO: Implement quiescence search
    (void)board;
    (void)alpha;
    (void)beta;
    return 0;
}

}  // namespace chess