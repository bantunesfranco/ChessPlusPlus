#pragma once

#include <functional>

#include "Move.hpp"
#include "Board.hpp"

namespace chess {

    class MoveGenerator {
    public:
        MoveGenerator() = default;

        /// Generate all legal moves for board position
        static void generate_all_moves(const Board& board, MoveList& moves) {
            board.generate_moves(moves);
        }

        /// Generate only captures and promotions
        static void generate_tactical_moves(const Board& board, MoveList& moves) {
            board.generate_captures(moves);
        }

        /// Check if a specific move is legal
        static bool is_legal(const Board& board, const Move& move) {
            return board.is_legal_move(move);
        }

        /// Filter moves: keep only those matching predicate
        static void filter_moves(MoveList& moves, const std::function<bool(Move)>& predicate) {
            MoveList filtered;
            for (const auto move : moves) {
                if (predicate(move)) {
                    filtered.add(move);
                }
            }
            moves = filtered;
        }
    };

} // namespace chess