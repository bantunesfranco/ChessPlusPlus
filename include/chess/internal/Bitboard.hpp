#pragma once

#include <cassert>

#include "chess/types.hpp"

namespace chess::internal {

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

    // // Pre-computed attack masks (non-sliding pieces)
    inline Bitboard KNIGHT_ATTACKS[64] = {};
    inline Bitboard KING_ATTACKS[64] = {};
    inline Bitboard PAWN_ATTACKS[2][64] = {};  // [color][square]

    void init_attacks();

}  // namespace chess::internal