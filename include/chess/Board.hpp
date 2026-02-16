#pragma once

#include <memory>
#include <optional>
#include <vector>

#include "types.hpp"
#include "Move.hpp"

namespace chess {

    class Board {
public:
    Board();
    ~Board();
    Board(const Board& board);
    Board& operator=(const Board& other);

    // === Initialization ===

    /// Load position from FEN string
    /// @param fen Forsyth-Edwards Notation string
    /// @throws std::invalid_argument if FEN is malformed
    void load_fen(const std::string& fen) const;

    /// Get current position as FEN
    /// @return FEN string representation
    [[nodiscard]] std::string to_fen() const;

    /// Reset to standard starting position
    void reset() const;

    // === Board Queries ===

    /// Get piece at square
    [[nodiscard]] Piece piece_at(Square sq) const;

    /// Get all pieces of type for color
    [[nodiscard]] std::vector<Square> pieces_of_type(Color color, PieceType type) const;

    /// Get all pieces for a color
    [[nodiscard]] std::vector<Square> pieces_of_color(Color color) const;

    /// Whose turn is it?
    [[nodiscard]] Color side_to_move() const;

    /// Can this color castle kingside?
    [[nodiscard]] bool can_castle_kingside(Color color) const;

    /// Can this color castle queenside?
    [[nodiscard]] bool can_castle_queenside(Color color) const;

    /// Get en passant target square (or INVALID if none)
    [[nodiscard]] Square en_passant_square() const;

    /// Get halfmove clock (for 50-move rule)
    [[nodiscard]] int halfmove_clock() const;

    /// Get fullmove number
    [[nodiscard]] int fullmove_number() const;

    // === Move Handling ===

    /// Generate all legal moves
    /// @param moves MoveList to fill with legal moves
    void generate_moves(MoveList& moves) const;

    /// Generate only capture moves
    void generate_captures(MoveList& moves) const;
    void generate_checks(MoveList& moves) const;

    /// Check if a move is legal
    [[nodiscard]] bool is_legal_move(Move move) const;

    /// Make a move (modifies board state)
    /// @throws std::invalid_argument if move is illegal
    void make_move(Move move);

    /// Undo the last move
    /// @throws std::runtime_error if no moves to undo
    void undo_move();

    /// Get move history
    [[nodiscard]] std::vector<Move> move_history() const;

    /// Clear move history
    void clear_history() const;

    // === Game State Queries ===

    /// Is the current side in check?
    [[nodiscard]] bool is_in_check() const;

    /// Is current position checkmate?
    [[nodiscard]] bool is_checkmate() const;

    /// Is current position a draw?
    [[nodiscard]] bool is_draw() const;

    /// Is current position stalemate?
    [[nodiscard]] bool is_stalemate() const;

    /// Has game reached 50-move rule draw?
    [[nodiscard]] bool is_50_move_draw() const;

    /// Count repetitions of current position in history
    [[nodiscard]] int position_repetitions() const;

    /// Is current position a threefold repetition?
    [[nodiscard]] bool is_threefold_repetition() const;

    /// Is game over? (checkmate, stalemate, draws)
    [[nodiscard]] bool is_game_over() const;

    /// Get game result: 1.0 (white win), 0.5 (draw), 0.0 (black win), none if ongoing
    [[nodiscard]] std::optional<double> game_result() const;

    // === Display ===

    /// ASCII representation of board
    [[nodiscard]] std::string to_string() const;

    // === Advanced Queries ===

    /// Get piece square hash (for transposition table, if needed)
    [[nodiscard]] Hash zobrist_hash() const;

    /// Check if position is legal (no double checks, etc)
    [[nodiscard]] bool is_valid_position() const;
    std::string move_to_san(const Move& move);

    // === Stream Operator ===

private:
    // Internal state (opaque to user)
    class Impl;
    std::unique_ptr<Impl> impl{};
};

}  // namespace chess

std::ostream& operator<<(std::ostream& os, const chess::Board& board);
