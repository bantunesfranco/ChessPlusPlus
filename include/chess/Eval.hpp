#pragma once

#include "types.hpp"
#include "Board.hpp"
#include "PieceSquareTables.hpp"

namespace chess {

    class Evaluator {
    public:
        Evaluator();
        explicit Evaluator(const PieceSquareTables& pst);
        ~Evaluator();

        /// Evaluate position from perspective of side to move
        /// Positive = side to move winning, Negative = side to move losing
        /// @param board Position to evaluate
        /// @return Score in centipawns
        Score evaluate(const Board& board) const;

        /// Evaluate from white's perspective (always)
        /// @return Score in centipawns (positive = white winning)
        // Score evaluate_white(const Board& board) const;

        /// Get material balance (in centipawns)
        /// Simple piece count without positional factors
        Score material_count(const Board& board) const;
        Score total_material_count(const Board& board) const;

        /// Estimate phase: 0.0 (endgame) to 1.0 (midgame opening)
        double get_phase(const Board& board) const;

    private:
        class Impl;
        std::unique_ptr<Impl> impl;
    };

}  // namespace chess