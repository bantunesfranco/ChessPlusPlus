#pragma once

#include <algorithm>
#include <vector>

#include "Move.hpp"
#include "types.hpp"

namespace chess {

    enum Flag { EXACT, LOWER_BOUND, UPPER_BOUND };

    struct TTEntry {
        Hash key;           // 64-bit Zobrist hash for position verification
        Score score;        // Evaluation score
        Depth depth;        // Search depth this entry was calculated at
        Flag flag;          // EXACT, LOWER_BOUND, or UPPER_BOUND
        Move best_move;     // Best move found at this position

        [[nodiscard]] bool matches(const Hash h, const Depth d) const {
            return key == h && depth >= d;
        }
    };

    class TranspositionTable {
    private:
        std::vector<TTEntry> table;
        size_t mask;

    public:
        explicit TranspositionTable(const size_t mb_size): mask(0) {
            resize(mb_size);
        }

        void store(const Hash h, const Score s, const Depth d, const Flag f, const Move m) {
            TTEntry& entry = table[h & mask];
            entry = {h, s, d, f, m};
        }

        [[nodiscard]] std::optional<TTEntry> lookup(const Hash h, const Depth d) const {
            if (const TTEntry& entry = table[h & mask]; entry.key == h && entry.depth >= d)
                return entry;
            return std::nullopt;
        }

        void resize(const size_t mb_size) {
            size_t entry_count = (mb_size * 1024 * 1024) / sizeof(TTEntry);
            // Round down to nearest power of 2
            entry_count = 1LL << (63 - __builtin_clzll(entry_count));
            table.resize(entry_count);
            mask = entry_count - 1;
        }

        void clear() { std::ranges::fill(table, TTEntry{}); }
        [[nodiscard]] size_t size_mb() const { return (table.size() * sizeof(TTEntry)) / (1024 * 1024); }
    };

}  // namespace chess