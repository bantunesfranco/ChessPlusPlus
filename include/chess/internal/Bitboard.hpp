#pragma once

#include <cassert>

#include "chess/types.hpp"

namespace chess::internal {

    static constexpr inline Bitboard BB_EMPTY = 0;
    static constexpr inline Bitboard BB_ALL = 0xFFFFFFFFFFFFFFFF;

    static constexpr inline Bitboard BB_A1 = 1ULL << A1;
    static constexpr inline Bitboard BB_B1 = 1ULL << B1;
    static constexpr inline Bitboard BB_C1 = 1ULL << C1;
    static constexpr inline Bitboard BB_D1 = 1ULL << D1;
    static constexpr inline Bitboard BB_E1 = 1ULL << E1;
    static constexpr inline Bitboard BB_F1 = 1ULL << F1;
    static constexpr inline Bitboard BB_G1 = 1ULL << G1;
    static constexpr inline Bitboard BB_H1 = 1ULL << H1;
    static constexpr inline Bitboard BB_A2 = 1ULL << A2;
    static constexpr inline Bitboard BB_B2 = 1ULL << B2;
    static constexpr inline Bitboard BB_C2 = 1ULL << C2;
    static constexpr inline Bitboard BB_D2 = 1ULL << D2;
    static constexpr inline Bitboard BB_E2 = 1ULL << E2;
    static constexpr inline Bitboard BB_F2 = 1ULL << F2;
    static constexpr inline Bitboard BB_G2 = 1ULL << G2;
    static constexpr inline Bitboard BB_H2 = 1ULL << H2;
    static constexpr inline Bitboard BB_A3 = 1ULL << A3;
    static constexpr inline Bitboard BB_B3 = 1ULL << B3;
    static constexpr inline Bitboard BB_C3 = 1ULL << C3;
    static constexpr inline Bitboard BB_D3 = 1ULL << D3;
    static constexpr inline Bitboard BB_E3 = 1ULL << E3;
    static constexpr inline Bitboard BB_F3 = 1ULL << F3;
    static constexpr inline Bitboard BB_G3 = 1ULL << G3;
    static constexpr inline Bitboard BB_H3 = 1ULL << H3;
    static constexpr inline Bitboard BB_A4 = 1ULL << A4;
    static constexpr inline Bitboard BB_B4 = 1ULL << B4;
    static constexpr inline Bitboard BB_C4 = 1ULL << C4;
    static constexpr inline Bitboard BB_D4 = 1ULL << D4;
    static constexpr inline Bitboard BB_E4 = 1ULL << E4;
    static constexpr inline Bitboard BB_F4 = 1ULL << F4;
    static constexpr inline Bitboard BB_G4 = 1ULL << G4;
    static constexpr inline Bitboard BB_H4 = 1ULL << H4;
    static constexpr inline Bitboard BB_A5 = 1ULL << A5;
    static constexpr inline Bitboard BB_B5 = 1ULL << B5;
    static constexpr inline Bitboard BB_C5 = 1ULL << C5;
    static constexpr inline Bitboard BB_D5 = 1ULL << D5;
    static constexpr inline Bitboard BB_E5 = 1ULL << E5;
    static constexpr inline Bitboard BB_F5 = 1ULL << F5;
    static constexpr inline Bitboard BB_G5 = 1ULL << G5;
    static constexpr inline Bitboard BB_H5 = 1ULL << H5;
    static constexpr inline Bitboard BB_A6 = 1ULL << A6;
    static constexpr inline Bitboard BB_B6 = 1ULL << B6;
    static constexpr inline Bitboard BB_C6 = 1ULL << C6;
    static constexpr inline Bitboard BB_D6 = 1ULL << D6;
    static constexpr inline Bitboard BB_E6 = 1ULL << E6;
    static constexpr inline Bitboard BB_F6 = 1ULL << F6;
    static constexpr inline Bitboard BB_G6 = 1ULL << G6;
    static constexpr inline Bitboard BB_H6 = 1ULL << H6;
    static constexpr inline Bitboard BB_A7 = 1ULL << A7;
    static constexpr inline Bitboard BB_B7 = 1ULL << B7;
    static constexpr inline Bitboard BB_C7 = 1ULL << C7;
    static constexpr inline Bitboard BB_D7 = 1ULL << D7;
    static constexpr inline Bitboard BB_E7 = 1ULL << E7;
    static constexpr inline Bitboard BB_F7 = 1ULL << F7;
    static constexpr inline Bitboard BB_G7 = 1ULL << G7;
    static constexpr inline Bitboard BB_H7 = 1ULL << H7;
    static constexpr inline Bitboard BB_A8 = 1ULL << A8;
    static constexpr inline Bitboard BB_B8 = 1ULL << B8;
    static constexpr inline Bitboard BB_C8 = 1ULL << C8;
    static constexpr inline Bitboard BB_D8 = 1ULL << D8;
    static constexpr inline Bitboard BB_E8 = 1ULL << E8;
    static constexpr inline Bitboard BB_F8 = 1ULL << F8;
    static constexpr inline Bitboard BB_G8 = 1ULL << G8;
    static constexpr inline Bitboard BB_H8 = 1ULL << H8;

    static constexpr inline Bitboard BB_CORNERS = BB_A1 | BB_H1 | BB_A8 | BB_H8;
    static constexpr inline Bitboard BB_CENTER = BB_D4 | BB_E4 | BB_D5 | BB_E5;

    static constexpr inline Bitboard BB_LIGHT_SQUARES = 0x55AA55AA55AA55AA;
    static constexpr inline Bitboard BB_DARK_SQUARES = 0xAA55AA55AA55AA55;

    static constexpr inline Bitboard BB_FILE_A = 0x0101010101010101 << FILE_A;
    static constexpr inline Bitboard BB_FILE_B = 0x0101010101010101 << FILE_B;
    static constexpr inline Bitboard BB_FILE_C = 0x0101010101010101 << FILE_C;
    static constexpr inline Bitboard BB_FILE_D = 0x0101010101010101 << FILE_D;
    static constexpr inline Bitboard BB_FILE_E = 0x0101010101010101 << FILE_E;
    static constexpr inline Bitboard BB_FILE_F = 0x0101010101010101 << FILE_F;
    static constexpr inline Bitboard BB_FILE_G = 0x0101010101010101 << FILE_G;
    static constexpr inline Bitboard BB_FILE_H = 0x0101010101010101 << FILE_H;

    static constexpr inline std::array<Bitboard, 8> BB_FILES = {
        BB_FILE_A, BB_FILE_B, BB_FILE_C, BB_FILE_D, BB_FILE_E, BB_FILE_F , BB_FILE_G, BB_FILE_H
    };
    static constexpr inline Bitboard BB_RANK_1 = 0xff << (8 * RANK_1)
    static constexpr inline Bitboard BB_RANK_2 = 0xff << (8 * RANK_2)
    static constexpr inline Bitboard BB_RANK_3 = 0xff << (8 * RANK_3)
    static constexpr inline Bitboard BB_RANK_4 = 0xff << (8 * RANK_4)
    static constexpr inline Bitboard BB_RANK_5 = 0xff << (8 * RANK_5)
    static constexpr inline Bitboard BB_RANK_6 = 0xff << (8 * RANK_6)
    static constexpr inline Bitboard BB_RANK_7 = 0xff << (8 * RANK_7)
    static constexpr inline Bitboard BB_RANK_8 = 0xff << (8 * RANK_8)
    static constexpr inline Bitboard BB_RANKS: List[Bitboard] = [BB_RANK_1, BB_RANK_2, BB_RANK_3, BB_RANK_4, BB_RANK_5, BB_RANK_6,
    static constexpr inline Bitboard BB_RANK_7, BB_RANK_8]
    static constexpr inline Bitboard BB_BACKRANKS: Bitboard = BB_RANK_1 | BB_RANK_8




    /// Get bit at square
    [[nodiscard]] inline bool get_bit(const Bitboard bb, Square sq) {
        return (bb >> static_cast<int>(sq)) & 1ULL;
    }

    /// Toggle a bit at square
    inline void toggle_bit(Bitboard& bb, Square sq) {
        bb ^= (1ULL << static_cast<int>(sq));
    }

    // Core bit operations
    [[nodiscard]] inline Bitboard set_bit(const Bitboard bb, const Square sq) {
        return bb | (1ULL << (int)sq);
    }

    [[nodiscard]] inline Bitboard clear_bit(const Bitboard bb, const Square sq) {
        return bb & ~(1ULL << (int)sq);
    }

    // Bit counting
    [[nodiscard]] inline int popcount(const Bitboard bb) {
        return __builtin_popcountll(bb);
    }

    // Find least significant bit (LSB)
    [[nodiscard]] inline int lsb(const Bitboard bb) {
        assert(bb != 0);
        return __builtin_ctzll(bb);  // Count trailing zeros
    }

    // Find most significant bit (MSB)
    [[nodiscard]] inline int msb(const Bitboard bb) {
        assert(bb != 0);
        return 63 - __builtin_clzll(bb);  // Count leading zeros
    }

    // Pop LSB and return its index
    [[nodiscard]] inline int pop_lsb(Bitboard& bb) {
        const int idx = lsb(bb);
        bb &= bb - 1;  // Clear LSB
        return idx;
    }

    [[nodiscard]] inline Bitboard rook_attacks(Square sq, Bitboard occupancy) {
        Bitboard attacks = 0;

        const int s = static_cast<int>(sq);
        const int file = s & 7;
        const int rank = s >> 3;

        // North
        for (int r = rank + 1; r < 8; ++r) {
            const int t = file + (r << 3);
            attacks |= 1ULL << t;
            if (occupancy & (1ULL << t)) break;
        }

        // South
        for (int r = rank - 1; r >= 0; --r) {
            const int t = file + (r << 3);
            attacks |= 1ULL << t;
            if (occupancy & (1ULL << t)) break;
        }

        // East
        for (int f = file + 1; f < 8; ++f) {
            const int t = f + (rank << 3);
            attacks |= 1ULL << t;
            if (occupancy & (1ULL << t)) break;
        }

        // West
        for (int f = file - 1; f >= 0; --f) {
            const int t = f + (rank << 3);
            attacks |= 1ULL << t;
            if (occupancy & (1ULL << t)) break;
        }

        return attacks;
    }

    [[nodiscard]] inline Bitboard bishop_attacks(Square sq, Bitboard occupancy) {
        Bitboard attacks = 0;

        const int s = static_cast<int>(sq);
        const int file = s & 7;
        const int rank = s >> 3;

        // NE
        for (int f = file + 1, r = rank + 1; f < 8 && r < 8; ++f, ++r) {
            const int t = f + (r << 3);
            attacks |= 1ULL << t;
            if (occupancy & (1ULL << t)) break;
        }

        // NW
        for (int f = file - 1, r = rank + 1; f >= 0 && r < 8; --f, ++r) {
            const int t = f + (r << 3);
            attacks |= 1ULL << t;
            if (occupancy & (1ULL << t)) break;
        }

        // SE
        for (int f = file + 1, r = rank - 1; f < 8 && r >= 0; ++f, --r) {
            const int t = f + (r << 3);
            attacks |= 1ULL << t;
            if (occupancy & (1ULL << t)) break;
        }

        // SW
        for (int f = file - 1, r = rank - 1; f >= 0 && r >= 0; --f, --r) {
            const int t = f + (r << 3);
            attacks |= 1ULL << t;
            if (occupancy & (1ULL << t)) break;
        }

        return attacks;
    }

    /// Generate queen attacks (combination of rook and bishop)
    [[nodiscard]] inline Bitboard queen_attacks(const Square sq, const Bitboard occupancy) {
        return rook_attacks(sq, occupancy) | bishop_attacks(sq, occupancy);
    }

    inline Bitboard pawn_attacks_to_square_white(Square sq) {
        Bitboard attacks = 0ULL;
        Bitboard sq_bb = 1ULL << static_cast<int>(sq);

        // Capture from left (from white's perspective)
        if ((sq_bb & FILE_H) == 0) // not on H file
            attacks |= sq_bb >> 7;

        // Capture from right
        if ((sq_bb & FILE_A) == 0) // not on A file
            attacks |= sq_bb >> 9;

        return attacks;
    }

    inline Bitboard pawn_attacks_to_square_black(Square sq) {
        Bitboard attacks = 0ULL;
        Bitboard sq_bb = 1ULL << static_cast<int>(sq);

        // Capture from left (from black's perspective)
        if ((sq_bb & FILE_H) == 0) // not on H file
            attacks |= sq_bb << 9;

        // Capture from right
        if ((sq_bb & FILE_A) == 0) // not on A file
            attacks |= sq_bb << 7;

        return attacks;
    }

    // // Pre-computed attack masks (non-sliding pieces)
    inline Bitboard KNIGHT_ATTACKS[64] = {};
    inline Bitboard KING_ATTACKS[64] = {};
    inline Bitboard PAWN_ATTACKS[2][64] = {};  // [color][square]

    void init_attacks();

}  // namespace chess::internal