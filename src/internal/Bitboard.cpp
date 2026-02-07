#include <cmath>

#include "../../include/chess/internal/Bitboard.hpp"

namespace chess::internal {

    void init_knight_attacks() {
        for (int sq = 0; sq < 64; ++sq) {
            Bitboard attacks = 0;
            // Knight can move to 8 relative positions: ±2 files ±1 rank or ±1 file ±2 ranks
            const int file = sq % 8;
            const int rank = sq / 8;

            constexpr int knight_offsets[8][2] = {{2, 1}, {2, -1}, {-2, 1}, {-2, -1},
                                                  {1, 2}, {1, -2}, {-1, 2}, {-1, -2}};

            for (const auto& [df, dr] : knight_offsets)
            {
                const int target_file = file + df;
                const int target_rank = rank + dr;

                if (target_file >= 0 && target_file < 8 &&
                    target_rank >= 0 && target_rank < 8) {
                    const int target_sq = target_file + target_rank * 8;
                    toggle_bit(attacks, (Square)target_sq);
                }
            }
            // KNIGHT_ATTACKS[sq] = attacks;
            memcpy(&KNIGHT_ATTACKS[sq] , &attacks, sizeof(Bitboard) );
        }
    }

    void init_king_attacks() {
        for (int sq = 0; sq < 64; ++sq) {
            Bitboard attacks = 0;
            const int file = sq % 8;
            const int rank = sq / 8;

            // King can move one square in all 8 directions
            for (int df = -1; df <= 1; ++df) {
                for (int dr = -1; dr <= 1; ++dr) {
                    if (df == 0 && dr == 0) continue;

                    const int target_file = file + df;
                    const int target_rank = rank + dr;

                    if (target_file >= 0 && target_file < 8 &&
                        target_rank >= 0 && target_rank < 8) {
                        const int target_sq = target_file + target_rank * 8;
                        toggle_bit(attacks, (Square)target_sq);
                    }
                }
            }
            // KING_ATTACKS[sq] = attacks;
            memcpy(&KING_ATTACKS[sq] , &attacks, sizeof(Bitboard) );
        }
    }

    void init_pawn_attacks() {
        for (int sq = 0; sq < 64; ++sq) {
            const int file = sq % 8;
            const int rank = sq / 8;

            // White pawn attacks (moving up, rank increases)
            Bitboard white_attacks = 0;
            if (rank < 7) {  // Not on rank 8
                if (file > 0) toggle_bit(white_attacks, (Square)(sq + 7));   // Left diagonal
                if (file < 7) toggle_bit(white_attacks, (Square)(sq + 9));   // Right diagonal
            }
            // PAWN_ATTACKS[0][sq] = white_attacks;
            memcpy(&PAWN_ATTACKS[0][sq] , &white_attacks, sizeof(Bitboard) );

            // Black pawn attacks (moving down, rank decreases)
            Bitboard black_attacks = 0;
            if (rank > 0) {  // Not on rank 1
                if (file > 0) toggle_bit(black_attacks, (Square)(sq - 9));   // Left diagonal
                if (file < 7) toggle_bit(black_attacks, (Square)(sq - 7));   // Right diagonal
            }
            // PAWN_ATTACKS[1][sq] = black_attacks;
            memcpy(&PAWN_ATTACKS[1][sq] , &black_attacks, sizeof(Bitboard) );
        }
    }

    void init_attacks() {
        init_knight_attacks();
        init_king_attacks();
        init_pawn_attacks();
    }

}  // namespace chess::internal