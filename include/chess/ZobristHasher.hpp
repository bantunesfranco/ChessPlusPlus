#pragma once

#include <array>
#include <random>

#include "types.hpp"
#include "internal/Bitboard.hpp"

namespace chess
{
    inline Piece make_piece(Color color, PieceType type) {
        return (Piece)((int)color * 6 + (int)type);
    }

    class ZobristHasher {
    private:
        static inline std::array<std::array<Hash, 64>, 12> piece_hashes{};   // [piece][square]
        static inline std::array<Hash, 16> castle_hashes{};                  // [castle_rights]
        static inline std::array<Hash, 8> en_passant_hashes{};               // [en_passant_file] (no hash if none)
        static inline Hash black_move_hash;

    public:
        ZobristHasher() {
            // Initialize with pseudo-random values (seed with fixed constant)
            std::mt19937_64 rng(0x123456789ABCDEFULL);
            std::uniform_int_distribution<Hash> dist;

            for (auto& row : piece_hashes) {
                for (auto& h : row) h = dist(rng);
            }
            for (auto& h : castle_hashes) h = dist(rng);
            for (auto& h : en_passant_hashes) h = dist(rng);
            black_move_hash = dist(rng);
        }

        [[nodiscard]] static Hash compute(const Position& pos)
        {
            Hash h = 0;

            // Hash pieces
            for (int color = 0; color < 2; ++color) {
                for (int piece = 0; piece < 6; ++piece) {
                    Bitboard bb = pos.pieces[color][piece];
                    while (bb) {
                        const int sq = internal::lsb(bb);
                        h ^= piece_hashes[color * 6 + piece][sq];
                        bb &= bb - 1;  // Clear LSB
                    }
                }
            }

            // Hash castle rights
            h ^= castle_hashes[pos.castle_rights];

            // Hash en passant
            if (pos.en_passant_square != Square::INVALID)
                h ^= en_passant_hashes[(int)pos.en_passant_square % 8];

            // Hash side to move
            if (pos.side_to_move == Color::BLACK)
                h ^= black_move_hash;

            return h;
        }

        [[nodiscard]] static Hash update(
            Hash h,
            const Move& move,
            const Piece moved_piece, const Piece captured_piece,
            const uint8_t old_castle_rights, const uint8_t new_castle_rights,
            const Square old_en_passant, const Square new_en_passant
        )
        {
            // Remove piece from source
            h ^= piece_hashes[(int)moved_piece][(int)move.from()];

            // Handle promotion
            if (move.flag() == MoveFlag::PROMOTION) {
                Piece promoted = make_piece(get_piece_color(moved_piece), move.promotion());
                h ^= piece_hashes[(int)promoted][(int)move.to()];
            }
            else
                h ^= piece_hashes[(int)moved_piece][(int)move.to()];

            // Handle capture
            if (captured_piece != Piece::NONE) {
                h ^= piece_hashes[(int)captured_piece][(int)move.to()];
            }

            // Handle en passant capture
            if (move.flag() == MoveFlag::EN_PASSANT) {
                Piece captured_pawn = make_piece(
                    (get_piece_color(moved_piece) == Color::WHITE) ? Color::BLACK : Color::WHITE,
                    PieceType::PAWN
                );
                auto captured_sq = (Square)((int)move.to() - 8);  // One rank back
                if (get_piece_color(moved_piece) == Color::BLACK)
                    captured_sq = (Square)((int)move.to() + 8);
                h ^= piece_hashes[(int)captured_pawn][(int)captured_sq];
            }

            // Handle castling rook movement
            if (move.flag() == MoveFlag::CASTLING) {
                Piece rook = make_piece(get_piece_color(moved_piece), PieceType::ROOK);

                Square rook_from, rook_to;
                if (move.to() == Square::G1) {
                    rook_from = Square::H1;
                    rook_to = Square::F1;
                } else if (move.to() == Square::C1) {
                    rook_from = Square::A1;
                    rook_to = Square::D1;
                } else if (move.to() == Square::G8) {
                    rook_from = Square::H8;
                    rook_to = Square::F8;
                } else {
                    rook_from = Square::A8;
                    rook_to = Square::D8;
                }

                h ^= piece_hashes[(int)rook][(int)rook_from];
                h ^= piece_hashes[(int)rook][(int)rook_to];
            }

            // Update castle rights
            if (old_castle_rights != new_castle_rights) {
                h ^= castle_hashes[old_castle_rights];
                h ^= castle_hashes[new_castle_rights];
            }

            // Update en passant
            if (old_en_passant != new_en_passant) {
                if (old_en_passant != Square::INVALID)
                    h ^= en_passant_hashes[(int)old_en_passant % 8];
                if (new_en_passant != Square::INVALID)
                    h ^= en_passant_hashes[(int)new_en_passant % 8];
            }

            // Side to move ALWAYS changes
            h ^= black_move_hash;

            return h;
        }
    };

}
