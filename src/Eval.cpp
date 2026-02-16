#include "chess/Eval.hpp"

#include <bits/regex_constants.h>

#include "chess/PieceSquareTables.hpp"

namespace chess
{
    PieceType get_piece_type(const Piece p)
    {
        return (PieceType)((int)p % 6);
    }

    Color get_piece_color(const Piece p)
    {
        return (Color)((int)p / 6);
    }

    class Evaluator::Impl
    {
        private:
            const PieceSquareTables pst;

        public:
            Impl() : pst(PieceSquareTables()) {};
            explicit Impl(const PieceSquareTables& pst) : pst(pst) {}

            Score evaluate(const Board& board);
            Score get_material(const Board& board, Color color);
            double get_game_phase(const Board& board);

    };

    Score Evaluator::Impl::evaluate(const Board& board) {
        Score score = 0;
        const double phase = get_game_phase(board);

        if (board.is_checkmate()) // If white is to move and in checkmate, black won
            return (board.side_to_move() == Color::WHITE) ? -CHECKMATE : CHECKMATE;

        if (board.is_draw())
            return 0;

        // Evaluate material and position
        for (const Color color : {Color::WHITE, Color::BLACK}) {
            const int sign = (color == Color::WHITE) ? 1 : -1;

            for (int pt_idx = 0; pt_idx < 6; ++pt_idx) {
                const auto pt = static_cast<PieceType>(pt_idx);
                auto pieces = board.pieces_of_type(color, pt);
                for (const Square sq : pieces) {
                    // Material value
                    const Score material = PIECE_VALUES[(int)pt];

                    // PST value (phase-interpolated)
                    const int value = pst.get_value(pt, sq, color, phase);

                    score += (sign * (material + value));
                }
            }
        }

        return score;
    }

    Score Evaluator::Impl::get_material(const Board& board, const Color color)
    {
        Score material = 0;
        // const double phase = get_game_phase(board);
        for (const auto sq : board.pieces_of_color(color)) {
            const Piece p = board.piece_at(sq);
            const PieceType pt = get_piece_type(p);

            material += PIECE_VALUES[(int)pt];
        }
        return material;
    }

    double Evaluator::Impl::get_game_phase(const Board& board)
    {
        int pieces = 0;

        for (const Color color : {Color::WHITE, Color::BLACK}) {
            pieces += board.pieces_of_type(color, PieceType::KNIGHT).size();
            pieces += board.pieces_of_type(color, PieceType::BISHOP).size();
            pieces += board.pieces_of_type(color, PieceType::ROOK).size() * 2;
            pieces += board.pieces_of_type(color, PieceType::QUEEN).size() * 4;
        }

        pieces = std::min(pieces, 24);
        return static_cast<double>(pieces * 256) / 24.0;
    }


    Evaluator::Evaluator() : impl(std::make_unique<Impl>()) {}

    Evaluator::Evaluator(const PieceSquareTables& pst): impl(std::make_unique<Impl>(pst)) {}

    Evaluator::~Evaluator() = default;

    Score Evaluator::evaluate(const Board& board) const
    {
        return impl->evaluate(board);
    }

    // Score Evaluator::evaluate_white(const Board& board) const
    // {
    //     Score eval = impl->evaluate(board);
    //     if (board.side_to_move() == Color::WHITE)
    //         eval = -eval;
    //     return eval;
    // }

    Score Evaluator::material_count(const Board& board) const
    {
        return impl->get_material(board, board.side_to_move());
    }

    Score Evaluator::total_material_count(const Board& board) const
    {
        const Color player = board.side_to_move();
        const Color opponent = player == Color::WHITE ? Color::BLACK : Color::WHITE;
        if (player == Color::WHITE)
            return impl->get_material(board, player) - impl->get_material(board, opponent);
        return impl->get_material(board, opponent) - impl->get_material(board, player);
    }

    double Evaluator::get_phase(const Board& board) const
    {
        return impl->get_game_phase(board);
    }

}

















