#pragma once

#include "types.hpp"
#include "Board.hpp"
#include "Move.hpp"

#include <chrono>
#include <functional>
#include <vector>
#include <optional>

namespace chess {

inline constexpr int MAX_DEPTH = 32;

// ============================================================================
// Search Configuration & Results
// ============================================================================

struct SearchResult {
    Move best_move;
    Score score;
    Depth depth;
    uint64_t nodes_searched;
    double search_time;
};

struct SearchConfig {
    std::chrono::milliseconds time_limit = std::chrono::milliseconds(5000);
    int max_depth = 20;
    int tt_size_mb = 64;
    bool use_transposition_table = true;
    bool use_quiescence_search = true;
    bool use_move_ordering = true;
    std::function<void(const SearchResult&)> on_iteration_complete;
};

// ============================================================================
// Engine Interface
// ============================================================================

class Engine {
public:
    Engine();
    explicit Engine(const SearchConfig& config);
    ~Engine();

    void set_config(const SearchConfig& config) const ;
    [[nodiscard]] SearchConfig get_config() const;
    void set_tt_size(int mb) const;
    void clear_cache();

    SearchResult find_best_move(const Board& board, std::chrono::milliseconds time_limit);
    SearchResult find_best_move_depth(const Board& board, Depth max_depth);
    SearchResult find_best_move(const Board& board, int max_depth, std::chrono::milliseconds time_limit);

    Score evaluate(const Board& board);

    std::vector<Move> get_principal_variation(const Board& board, int depth);
    MoveList get_ranked_moves(const Board& board);

    struct Analysis {
        Move best_move;
        std::vector<Move> pv;
        Score score;
        Depth depth;
        std::vector<std::pair<Move, Score>> move_scores;
    };

    Analysis analyze(const Board& board, int depth);

    void set_progress_callback(const std::function<void(int depth, uint64_t nodes)>& callback);
    void stop_search();

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

}  // namespace chess