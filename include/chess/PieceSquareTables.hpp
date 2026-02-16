#pragma once

#include <algorithm>

#include "types.hpp"

#include <array>

namespace chess {

    // Bonuses for pawn placement (midgame)
    // White's perspective: rank 0 = rank 1, rank 7 = rank 8
    static constexpr int PAWN_MG[64] = {
        0,   0,   0,   0,   0,   0,   0,   0,
        2,   4,   5,  10,  10,   5,   4,   2,   // Rank 2: modest advance bonus
        4,   8,  12,  16,  16,  12,   8,   4,   // Rank 3: better advanced
        6,  12,  16,  24,  24,  16,  12,   6,   // Rank 4: center pawns stronger
        8,  16,  24,  32,  32,  24,  16,   8,   // Rank 5: passed pawn territory
       12,  24,  36,  48,  48,  36,  24,  12,   // Rank 6: critical advancement
        0,   0,   0,   0,   0,   0,   0,   0,   // Rank 7: promotion (handled specially)
        0,   0,   0,   0,   0,   0,   0,   0,   // Rank 8: shouldn't exist
    };

    // Bonuses for pawn placement (endgame)
    static constexpr int PAWN_EG[64] = {
        0,   0,   0,   0,   0,   0,   0,   0,
       10,  10,  10,  10,  10,  10,  10,  10,   // Rank 2: passed pawn bonus
       20,  20,  20,  20,  20,  20,  20,  20,   // Rank 3: higher bonus
       30,  30,  30,  30,  30,  30,  30,  30,   // Rank 4
       40,  40,  40,  40,  40,  40,  40,  40,   // Rank 5: strong advancement
       60,  60,  60,  60,  60,  60,  60,  60,   // Rank 6: very strong
      100, 100, 100, 100, 100, 100, 100, 100,   // Rank 7: nearly winning
        0,   0,   0,   0,   0,   0,   0,   0,   // Rank 8
    };

    // Knights love the center during middlegame
    static constexpr int KNIGHT_MG[64] = {
        -10,  -8,  -6,  -4,  -4,  -6,  -8, -10,
         -8,   0,   2,   4,   4,   2,   0,  -8,
         -6,   2,   6,   8,   8,   6,   2,  -6,
         -4,   4,   8,  10,  10,   8,   4,  -4,  // Rank 4: center squares
         -4,   4,   8,  10,  10,   8,   4,  -4,  // Rank 4: symmetric
         -6,   2,   6,   8,   8,   6,   2,  -6,
         -8,   0,   2,   4,   4,   2,   0,  -8,
        -10,  -8,  -6,  -4,  -4,  -6,  -8, -10,
     };

    // Endgame: knights still like center, but less critical
    static constexpr int KNIGHT_EG[64] = {
        -6,  -4,  -2,   0,   0,  -2,  -4,  -6,
        -4,   0,   2,   4,   4,   2,   0,  -4,
        -2,   2,   4,   6,   6,   4,   2,  -2,
         0,   4,   6,   8,   8,   6,   4,   0,
         0,   4,   6,   8,   8,   6,   4,   0,
        -2,   2,   4,   6,   6,   4,   2,  -2,
        -4,   0,   2,   4,   4,   2,   0,  -4,
        -6,  -4,  -2,   0,   0,  -2,  -4,  -6,
    };

    // Bishops control long diagonals; prefer open positions
    static constexpr int BISHOP_MG[64] = {
        -4,  -2,  -2,  -2,  -2,  -2,  -2,  -4,
        -2,   0,   2,   2,   2,   2,   0,  -2,
        -2,   2,   4,   4,   4,   4,   2,  -2,
        -2,   2,   4,   6,   6,   4,   2,  -2,  // Rank 4: center diagonals
        -2,   2,   4,   6,   6,   4,   2,  -2,  // Rank 4
        -2,   2,   4,   4,   4,   4,   2,  -2,
        -2,   0,   2,   2,   2,   2,   0,  -2,
        -4,  -2,  -2,  -2,  -2,  -2,  -2,  -4,
    };

    static constexpr int BISHOP_EG[64] = {
        -2,  -1,  -1,  -1,  -1,  -1,  -1,  -2,
        -1,   0,   1,   1,   1,   1,   0,  -1,
        -1,   1,   2,   2,   2,   2,   1,  -1,
        -1,   1,   2,   4,   4,   2,   1,  -1,
        -1,   1,   2,   4,   4,   2,   1,  -1,
        -1,   1,   2,   2,   2,   2,   1,  -1,
        -1,   0,   1,   1,   1,   1,   0,  -1,
        -2,  -1,  -1,  -1,  -1,  -1,  -1,  -2,
    };

    // Rooks want open files and 7th rank (behind enemy pawns)
    static constexpr int ROOK_MG[64] = {
        0,   1,   2,   3,   3,   2,   1,   0,
        1,   2,   3,   4,   4,   3,   2,   1,
        0,   0,   0,   0,   0,   0,   0,   0,  // Rank 3: neutral
        0,   0,   0,   0,   0,   0,   0,   0,  // Rank 4
        0,   0,   0,   0,   0,   0,   0,   0,  // Rank 5
        0,   0,   0,   0,   0,   0,   0,   0,  // Rank 6
        5,   5,   5,   5,   5,   5,   5,   5,  // Rank 7: 7th rank bonus!
        0,   1,   2,   3,   3,   2,   1,   0,
   };

    // Endgame: rooks more flexible, centralization matters
    static constexpr int ROOK_EG[64] = {
        -4,  -2,   0,   0,   0,   0,  -2,  -4,
        -2,   0,   2,   2,   2,   2,   0,  -2,
         0,   2,   4,   4,   4,   4,   2,   0,  // Rank 3: centralize
         0,   2,   4,   6,   6,   4,   2,   0,  // Rank 4
         0,   2,   4,   6,   6,   4,   2,   0,  // Rank 5
         0,   2,   4,   4,   4,   4,   2,   0,  // Rank 6
        -2,   0,   2,   2,   2,   2,   0,  -2,
        -4,  -2,   0,   0,   0,   0,  -2,  -4,
    };

    // Queens are flexible; generally centralize but not as aggressive
    static constexpr int QUEEN_MG[64] = {
        -4,  -2,   0,   0,   0,   0,  -2,  -4,
        -2,   0,   2,   2,   2,   2,   0,  -2,
         0,   2,   4,   4,   4,   4,   2,   0,
         0,   2,   4,   6,   6,   4,   2,   0,  // Rank 4: center bonus
         0,   2,   4,   6,   6,   4,   2,   0,  // Rank 5
         0,   2,   4,   4,   4,   4,   2,   0,
        -2,   0,   2,   2,   2,   2,   0,  -2,
        -4,  -2,   0,   0,   0,   0,  -2,  -4,
    };

    static constexpr int QUEEN_EG[64] = {
        -2,  -1,   0,   0,   0,   0,  -1,  -2,
        -1,   0,   1,   1,   1,   1,   0,  -1,
         0,   1,   2,   2,   2,   2,   1,   0,
         0,   1,   2,   4,   4,   2,   1,   0,
         0,   1,   2,   4,   4,   2,   1,   0,
         0,   1,   2,   2,   2,   2,   1,   0,
        -1,   0,   1,   1,   1,   1,   0,  -1,
        -2,  -1,   0,   0,   0,   0,  -1,  -2,
    };

    // During middlegame, king safety is CRITICAL
    // Incentivize staying in corner (castled position)
    static constexpr int KING_MG[64] = {
        -40, -30, -30, -30, -30, -30, -30, -40,  // Rank 8
        -30, -20, -10, -10, -10, -10, -20, -30,  // Rank 7
        -20, -10,   0,   0,   0,   0, -10, -20,  // Rank 6
        -10,   0,   5,   5,   5,   5,   0, -10,  // Rank 5
          0,   5,  10,  10,  10,  10,   5,   0,  // Rank 4
         -5,   0,   5,  10,  10,   5,   0,  -5,  // Rank 3
        -30, -20, -10, -10, -10, -10, -20, -30,  // Rank 2
        -50, -40, -30, -30, -30, -30, -40, -50,  // Rank 1
     };

    // Better representation: castled king bonuses
    static constexpr int KING_MG_CASTLED[64] = {
        // Kingside castled (king on g1): safe in corner
        0,   0,   0,   0,   0,   0,   0,   0,
        0,   0,   0,   0,   0,   0,   0,   0,
       10,  10,   5,   0,   0,   5,  10,  10,
        5,   5,   0,   0,   0,   0,   5,   5,
        0,   0,   0,   0,   0,   0,   0,   0,
      -10, -10,   0,   0,   0,   0, -10, -10,
      -20, -10,   0,   0,   0,   0, -10, -20,
      -50, -40, -30, -20, -20, -30, -40, -50,
    };

    // In endgame with few pieces, king becomes powerful
    // Incentivize king moving toward center
    static constexpr int KING_EG[64] = {
        -6,  -4,  -2,   0,   0,  -2,  -4,  -6,
        -4,   0,   2,   4,   4,   2,   0,  -4,
        -2,   2,   4,   6,   6,   4,   2,  -2,
         0,   4,   6,   8,   8,   6,   4,   0,  // Rank 4: center bonus
         0,   4,   6,   8,   8,   6,   4,   0,  // Rank 5
        -2,   2,   4,   6,   6,   4,   2,  -2,
        -4,   0,   2,   4,   4,   2,   0,  -4,
        -6,  -4,  -2,   0,   0,  -2,  -4,  -6,
    };

    class PieceSquareTables {
    private:
        // Tables for each piece type (endgame and midgame)
        std::array<std::array<int, 64>, 6> midgame_tables;
        std::array<std::array<int, 64>, 6> endgame_tables;

    public:
        PieceSquareTables() : midgame_tables(), endgame_tables() {
            // Initialize with evaluation bonuses
            for (int i = 0; i < 64; i++) midgame_tables[(int)PieceType::PAWN][i] = PAWN_MG[i];
            for (int i = 0; i < 64; i++) endgame_tables[(int)PieceType::PAWN][i] = PAWN_EG[i];

            for (int i = 0; i < 64; i++) midgame_tables[(int)PieceType::KNIGHT][i] = KNIGHT_MG[i];
            for (int i = 0; i < 64; i++) endgame_tables[(int)PieceType::KNIGHT][i] = KNIGHT_EG[i];

            for (int i = 0; i < 64; i++) midgame_tables[(int)PieceType::BISHOP][i] = BISHOP_MG[i];
            for (int i = 0; i < 64; i++) endgame_tables[(int)PieceType::BISHOP][i] = BISHOP_EG[i];

            for (int i = 0; i < 64; i++) midgame_tables[(int)PieceType::ROOK][i] = ROOK_MG[i];
            for (int i = 0; i < 64; i++) endgame_tables[(int)PieceType::ROOK][i] = ROOK_EG[i];

            for (int i = 0; i < 64; i++) midgame_tables[(int)PieceType::QUEEN][i] = QUEEN_MG[i];
            for (int i = 0; i < 64; i++) endgame_tables[(int)PieceType::QUEEN][i] = QUEEN_EG[i];

            for (int i = 0; i < 64; i++) midgame_tables[(int)PieceType::KING][i] = KING_MG[i];
            for (int i = 0; i < 64; i++) endgame_tables[(int)PieceType::KING][i] = KING_EG[i];
        }


        [[nodiscard]] int get_midgame_value(const PieceType pt, const Square sq, const Color c) const {
            if (c == Color::WHITE)
                return midgame_tables[(int)pt][(int)sq];

            // Flip rank only (not entire square)
            const int file = (int)sq % 8;
            const int rank = 7 - ((int)sq / 8);
            const int flipped = rank * 8 + file;
            return midgame_tables[(int)pt][flipped];
        }

        [[nodiscard]] int get_endgame_value(const PieceType pt, const Square sq, const Color c) const {
            if (c == Color::WHITE)
                return endgame_tables[(int)pt][(int)sq];

            // Flip rank only
            const int file = (int)sq % 8;
            const int rank = 7 - ((int)sq / 8);
            const int flipped = rank * 8 + file;
            return endgame_tables[(int)pt][flipped];
        }

        [[nodiscard]] int get_value(const PieceType pt, const Square sq, const Color c, const int phase) const {
            // Interpolate between midgame and endgame, phase ranges from 0 (endgame) to 256 (midgame)
            const int mg_score = get_midgame_value(pt, sq, c);
            const int eg_score = get_endgame_value(pt, sq, c);
            return (mg_score * phase + eg_score * (256 - phase)) / 256;
        }
    };
}
