#pragma once

#include <array>
#include <stdexcept>

#include "types.hpp"

namespace chess {

    class Move {
    private:
        uint32_t data = 0;  // Packed move data

    public:
        Move() = default;

        Move(Square from, Square to, MoveFlag flag = MoveFlag::NORMAL,
             PieceType promotion = PieceType::NONE) {
            data = (static_cast<int>(from)) |
                   (static_cast<int>(to) << 6) |
                   (static_cast<int>(flag) << 12) |
                   (static_cast<int>(promotion) << 15);
        }

        // Accessors
        [[nodiscard]] Square from() const { return Square(data & 0x3F); }
        [[nodiscard]] Square to() const { return Square((data >> 6) & 0x3F); }
        [[nodiscard]] MoveFlag flag() const { return MoveFlag((data >> 12) & 0x7); }
        [[nodiscard]] PieceType promotion() const { return PieceType((data >> 15) & 0x7); }

        // Query methods
        [[nodiscard]] bool is_capture() const { return flag() == MoveFlag::CAPTURE; }
        [[nodiscard]] bool is_promotion() const { return flag() == MoveFlag::PROMOTION; }
        [[nodiscard]] bool is_castling() const { return flag() == MoveFlag::CASTLING; }
        [[nodiscard]] bool is_en_passant() const { return flag() == MoveFlag::EN_PASSANT; }

        // Comparison
        bool operator==(const Move& other) const { return data == other.data; }
        bool operator!=(const Move& other) const { return data != other.data; }

        // Conversion to/from algebraic notation
        [[nodiscard]] std::string to_uci() const;
        static Move from_uci(const std::string& uci_str);

        [[nodiscard]] uint32_t raw() const { return data; }
    };

    // Move list for generator
    class MoveList {
    private:
        std::array<Move, 256> moves;
        size_t count = 0;

    public:
        void add(const Move m) { moves[count++] = m; }
        void clear() { count = 0; }

        [[nodiscard]] size_t size() const { return count; }
        [[nodiscard]] bool empty() const { return count == 0; }

        Move operator[](const size_t idx) const { return moves[idx]; }
        [[nodiscard]] Move at(const size_t idx) const {
            if (idx >= count) throw std::out_of_range("MoveList index out of range");
            return moves[idx];
        }

        // Iterator support
        auto begin() { return moves.begin(); }
        auto end() { return moves.begin() + count; }
        [[nodiscard]] auto begin() const { return moves.begin(); }
        [[nodiscard]] auto end() const { return moves.begin() + count; }
    };

    struct MoveUndo {
        Move move;
        Piece captured_piece;
        uint8_t old_castle_rights;
        Square old_en_passant;
        uint16_t old_halfmove_clock;
        Hash old_hash;
    };
}  // namespace chess