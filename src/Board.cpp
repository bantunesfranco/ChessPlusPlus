#include <format>
#include <sstream>

#include "chess/types.hpp"
#include "chess/Board.hpp"

#include <algorithm>
#include <ranges>
#include <unordered_map>

#include "chess/Move.hpp"
#include "chess/ZobristHasher.hpp"
#include "chess/internal/Bitboard.hpp"

using namespace chess;

namespace chess
{
    // ============================================================================
    // Helper Functions (internal)
    // ============================================================================

    Square string_to_square(const std::string& s) {
        if (s == "-") return Square::INVALID;
        if (s.length() != 2) throw std::invalid_argument("Invalid square");

        const int file = s[0] - 'a';  // 0-7
        const int rank = s[1] - '1';  // 0-7

        if (file < 0 || file > 7 || rank < 0 || rank > 7)
            throw std::invalid_argument("Invalid square");

        return (Square)(file + rank * 8);
    }

    std::string square_to_string(Square sq) {
        if (sq == Square::INVALID) return "-";

        const int file = (int)sq % 8;
        const int rank = (int)sq / 8;

        return std::string(1, (char)('a' + file)) + std::string(1, (char)('1' + rank));
    }

    char piece_to_char(Piece p) {
        static constexpr char chars[] = "PNBRQKpnbrqk";
        return chars[(int)p];
    }

    // ============================================================================
    // Board::Impl (internal)
    // ============================================================================

    class Board::Impl
    {
    public:
        Impl();
        Impl(const Impl& other) = default;
        Impl& operator=(const Impl& other) = default;

        Position position;
        ZobristHasher hasher;
        std::vector<MoveUndo> undo_history;

        static constexpr std::string_view DEFAULT_BOARD = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR";

        void reset();
        void parse_fen(const std::string& fen);
        [[nodiscard]] std::string position_to_fen() const;
        [[nodiscard]] std::string board_to_ascii() const;
        [[nodiscard]] Piece get_piece_at(Square sq) const;
        [[nodiscard]] std::vector<Square> pieces_of_color(Color color) const;
        [[nodiscard]] std::vector<Square> pieces_of_type(Color color, PieceType type) const;
        [[nodiscard]] std::vector<Move> get_move_history() const;

        void generate_pseudo_legal_moves(MoveList& moves) const;
        void generate_pawn_moves(Color color, MoveList& moves) const;
        void generate_knight_moves(Color color, MoveList& moves) const;
        void generate_king_moves(Color color, MoveList& moves) const;
        void generate_bishop_moves(Square sq, MoveList& moves) const;
        void generate_rook_moves(Square sq, MoveList& moves) const;
        void generate_queen_moves(Square sq, MoveList& moves) const;
        void generate_castling_moves(Color color, MoveList& moves) const;

        [[nodiscard]] bool is_square_attacked_by(Square sq, Color enemy_color) const;
        [[nodiscard]] bool is_king_under_attack(Color king_color) const;

        [[nodiscard]] uint8_t calculate_new_castle_rights(Move move) const;
        [[nodiscard]] Square calculate_new_en_passant(Move move);
        void apply_move(const Move& move);
        void restore_from_history();

        [[nodiscard]] Square find_king(Color color) const;
        void update_occupancy();

    private:
        void parse_board(const std::string_view& board_str, Bitboard pieces[2][6]);
        [[nodiscard]] std::string board_to_fen() const;
        [[nodiscard]] std::string castle_rights_to_string(uint8_t castle_rights) const;
    };

    Board::Impl::Impl() {
        internal::init_attacks();
    }

    std::vector<Move> Board::Impl::get_move_history() const {
        std::vector<Move> moves;
        for (const auto& undo : undo_history)
            moves.push_back(undo.move);
        return moves;
    }

    void Board::Impl::generate_pseudo_legal_moves(MoveList& moves) const {
        moves.clear();

        const Color player = position.side_to_move;

        // Generate for each piece type
        generate_pawn_moves(player, moves);
        generate_knight_moves(player, moves);
        generate_king_moves(player, moves);

        for (const Square sq : pieces_of_type(player, PieceType::BISHOP))
            generate_bishop_moves(sq, moves);

        for (const Square sq : pieces_of_type(player, PieceType::ROOK))
            generate_rook_moves(sq, moves);

        for (const Square sq : pieces_of_type(player, PieceType::QUEEN))
            generate_queen_moves(sq, moves);
    }

    void Board::Impl::generate_pawn_moves(Color color, MoveList& moves) const {
        Bitboard pawns = position.pieces[(int)color][(int)PieceType::PAWN];

        while (pawns) {
            int sq = internal::lsb(pawns);
            auto from = (Square)sq;

            // Pawn single push
            const int direction = (color == Color::WHITE) ? 8 : -8;
            if (auto to = (Square)((int)from + direction); (int)to < 64 && get_piece_at(to) == Piece::NONE) {
                // Check promotion
                if (const int rank = (int)to / 8; (color == Color::WHITE && rank == 7) || (color == Color::BLACK && rank == 0)) {
                    moves.add(Move(from, to, MoveFlag::PROMOTION, PieceType::QUEEN));
                    moves.add(Move(from, to, MoveFlag::PROMOTION, PieceType::ROOK));
                    moves.add(Move(from, to, MoveFlag::PROMOTION, PieceType::BISHOP));
                    moves.add(Move(from, to, MoveFlag::PROMOTION, PieceType::KNIGHT));
                } else {
                    moves.add(Move(from, to, MoveFlag::NORMAL));
                }

                // Double push from starting rank
                if ((color == Color::WHITE && ((int)from / 8) == 1) ||
                    (color == Color::BLACK && ((int)from / 8) == 6)) {
                    if (const auto double_to = (Square)((int)from + 2 * direction); get_piece_at(double_to) == Piece::NONE) {
                        moves.add(Move(from, double_to, MoveFlag::NORMAL));
                    }
                }
            }

            // Pawn captures
            Bitboard attacks = internal::PAWN_ATTACKS[(int)color][(int)sq] & ~position.occupancy[(int)color];
            while (attacks) {
                int to_sq = internal::lsb(attacks);
                auto to = (Square)to_sq;

                if (const Piece target = get_piece_at(to); target != Piece::NONE && get_piece_color(target) != color) {
                    // Check promotion capture
                    if (const int rank = (int)to / 8; (color == Color::WHITE && rank == 7) || (color == Color::BLACK && rank == 0)) {
                        moves.add(Move(from, to, MoveFlag::PROMOTION, PieceType::QUEEN));
                        moves.add(Move(from, to, MoveFlag::PROMOTION, PieceType::ROOK));
                        moves.add(Move(from, to, MoveFlag::PROMOTION, PieceType::BISHOP));
                        moves.add(Move(from, to, MoveFlag::PROMOTION, PieceType::KNIGHT));
                    } else {
                        moves.add(Move(from, to, MoveFlag::CAPTURE));
                    }
                }

                attacks &= attacks - 1;
            }

            pawns &= pawns - 1;
        }
    }

    void Board::Impl::generate_knight_moves(Color color, MoveList& moves) const {
        Bitboard knights = position.pieces[(int)color][(int)PieceType::KNIGHT];

        while (knights) {
            int sq = internal::lsb(knights);
            const auto from = (Square)sq;

            Bitboard attacks = internal::KNIGHT_ATTACKS[sq] & ~position.occupancy[(int)color];
            while (attacks) {
                int to_sq = internal::lsb(attacks);
                const auto to = (Square)to_sq;

                if (const Piece target = get_piece_at(to); target == Piece::NONE) {
                    moves.add(Move(from, to, MoveFlag::NORMAL));
                } else if (get_piece_color(target) != color) {
                    moves.add(Move(from, to, MoveFlag::CAPTURE));
                }

                attacks &= attacks - 1;
            }

            knights &= knights - 1;
        }
    }

    void Board::Impl::generate_king_moves(Color color, MoveList& moves) const {
        const Bitboard kings = position.pieces[(int)color][(int)PieceType::KING];

        if (kings == 0) return;  // No king (invalid position)

        int sq = internal::lsb(kings);
        const auto from = (Square)sq;

        Bitboard attacks = internal::KING_ATTACKS[sq] & ~position.occupancy[(int)color];
        while (attacks) {
            int to_sq = internal::lsb(attacks);
            const auto to = (Square)to_sq;

            if (const Piece target = get_piece_at(to); target == Piece::NONE) {
                moves.add(Move(from, to, MoveFlag::NORMAL));
            } else if (get_piece_color(target) != color) {
                moves.add(Move(from, to, MoveFlag::CAPTURE));
            }

            attacks &= attacks - 1;
        }

        generate_castling_moves(position.side_to_move, moves);
    }

    void Board::Impl::generate_castling_moves(const Color color, MoveList& moves) const {
        // White castling
        if (color == Color::WHITE) {
            // Kingside castling (e1 to g1)
            if ((position.castle_rights & 0b0001) != 0) {  // White kingside right exists
                // Check if f1 and g1 are empty
                if (get_piece_at(Square::F1) == Piece::NONE && get_piece_at(Square::G1) == Piece::NONE) {
                    // Check if e1, f1, g1 are not under attack
                    if (!is_square_attacked_by(Square::E1, Color::BLACK) &&
                        !is_square_attacked_by(Square::F1, Color::BLACK) &&
                        !is_square_attacked_by(Square::G1, Color::BLACK)) {
                        moves.add(Move(Square::E1, Square::G1, MoveFlag::CASTLING));
                    }
                }
            }

            // Queenside castling (e1 to c1)
            if ((position.castle_rights & 0b0010) != 0) {  // White queenside right exists
                // Check if b1, c1, d1 are empty
                if (get_piece_at(Square::B1) == Piece::NONE &&
                    get_piece_at(Square::C1) == Piece::NONE &&
                    get_piece_at(Square::D1) == Piece::NONE) {
                    // Check if e1, d1, c1 are not under attack
                    if (!is_square_attacked_by(Square::E1, Color::BLACK) &&
                        !is_square_attacked_by(Square::D1, Color::BLACK) &&
                        !is_square_attacked_by(Square::C1, Color::BLACK)) {
                        moves.add(Move(Square::E1, Square::C1, MoveFlag::CASTLING));
                    }
                }
            }
        }

        // Black castling
        if (color == Color::BLACK) {
            // Kingside castling (e8 to g8)
            if ((position.castle_rights & 0b0100) != 0) {  // Black kingside right exists
                // Check if f8 and g8 are empty
                if (get_piece_at(Square::F8) == Piece::NONE && get_piece_at(Square::G8) == Piece::NONE) {
                    // Check if e8, f8, g8 are not under attack
                    if (!is_square_attacked_by(Square::E8, Color::WHITE) &&
                        !is_square_attacked_by(Square::F8, Color::WHITE) &&
                        !is_square_attacked_by(Square::G8, Color::WHITE)) {
                        moves.add(Move(Square::E8, Square::G8, MoveFlag::CASTLING));
                    }
                }
            }

            // Queenside castling (e8 to c8)
            if ((position.castle_rights & 0b1000) != 0) {  // Black queenside right exists
                // Check if b8, c8, d8 are empty
                if (get_piece_at(Square::B8) == Piece::NONE &&
                    get_piece_at(Square::C8) == Piece::NONE &&
                    get_piece_at(Square::D8) == Piece::NONE) {
                    // Check if e8, d8, c8 are not under attack
                    if (!is_square_attacked_by(Square::E8, Color::WHITE) &&
                        !is_square_attacked_by(Square::D8, Color::WHITE) &&
                        !is_square_attacked_by(Square::C8, Color::WHITE)) {
                        moves.add(Move(Square::E8, Square::C8, MoveFlag::CASTLING));
                    }
                }
            }
        }
    }

    void Board::Impl::generate_bishop_moves(const Square sq, MoveList& moves) const {
        Bitboard attacks = internal::bishop_attacks(sq, position.occupancy_all) & ~position.occupancy[(int)position.side_to_move];
        while (attacks) {
            int to_sq = internal::lsb(attacks);
            const auto to = (Square)to_sq;

            if (const Piece target = get_piece_at(to); target == Piece::NONE) {
                moves.add(Move(sq, to, MoveFlag::NORMAL));
            } else if (get_piece_color(target) != position.side_to_move) {
                moves.add(Move(sq, to, MoveFlag::CAPTURE));
            }

            attacks &= attacks - 1;
        }
    }

    void Board::Impl::generate_rook_moves(const Square sq, MoveList& moves) const {
        Bitboard attacks = internal::rook_attacks(sq, position.occupancy_all) & ~position.occupancy[(int)position.side_to_move];
        while (attacks) {
            int to_sq = internal::lsb(attacks);
            const auto to = (Square)to_sq;

            if (const Piece target = get_piece_at(to); target == Piece::NONE) {
                moves.add(Move(sq, to, MoveFlag::NORMAL));
            } else if (get_piece_color(target) != position.side_to_move) {
                moves.add(Move(sq, to, MoveFlag::CAPTURE));
            }

            attacks &= attacks - 1;
        }
    }

    void Board::Impl::generate_queen_moves(const Square sq, MoveList& moves) const {
        Bitboard attacks = internal::queen_attacks(sq, position.occupancy_all) & ~position.occupancy[(int)position.side_to_move];
        while (attacks) {
            int to_sq = internal::lsb(attacks);
            const auto to = (Square)to_sq;

            if (const Piece target = get_piece_at(to); target == Piece::NONE) {
                moves.add(Move(sq, to, MoveFlag::NORMAL));
            } else if (get_piece_color(target) != position.side_to_move) {
                moves.add(Move(sq, to, MoveFlag::CAPTURE));
            }

            attacks &= attacks - 1;
        }
    }

    bool Board::Impl::is_square_attacked_by(Square sq, Color enemy_color) const
    {
        using namespace internal;
        if (KNIGHT_ATTACKS[(int)sq] & position.pieces[(int)enemy_color][(int)PieceType::KNIGHT])
            return true;

        if (KING_ATTACKS[(int)sq] & position.pieces[(int)enemy_color][(int)PieceType::KING])
            return true;

        if (PAWN_ATTACKS[(int)enemy_color][(int)sq] & position.pieces[(int)enemy_color][(int)PieceType::PAWN])
            return true;

        if (rook_attacks(sq, position.occupancy_all) & position.pieces[(int)enemy_color][(int)PieceType::ROOK])
            return true;

        if (bishop_attacks(sq, position.occupancy_all) & position.pieces[(int)enemy_color][(int)PieceType::BISHOP])
            return true;

        if (queen_attacks(sq, position.occupancy_all) & position.pieces[(int)enemy_color][(int)PieceType::QUEEN])
                return true;

        return false;
    }

    bool Board::Impl::is_king_under_attack(const Color king_color) const {
        const Square king = find_king(king_color);
        const Color enemy_color = (king_color == Color::WHITE) ? Color::BLACK : Color::WHITE;

        if (king == Square::INVALID)
            throw std::invalid_argument("Invalid board: no king found");

        return is_square_attacked_by(king, enemy_color);
    }

    uint8_t Board::Impl::calculate_new_castle_rights(const Move move) const {
        const Square from = move.from();
        const Square to = move.to();
        if (move.flag() == MoveFlag::CASTLING) {
            // If it's a castling move, we know the rights will be lost
            if (from == Square::E1) return position.castle_rights & 0b1100;  // White loses both
            if (from == Square::E8) return position.castle_rights & 0b0011;  // Black loses both
        }
        if (from == Square::H1 || to == Square::H1) return position.castle_rights & 0b1110;  // White kingside lost
        if (from == Square::A1 || to == Square::A1) return position.castle_rights & 0b1101;  // White queenside lost
        if (from == Square::H8 || to == Square::H8) return position.castle_rights & 0b1011;  // Black kingside lost
        if (from == Square::A8 || to == Square::A8) return position.castle_rights & 0b0111;  // Black queenside lost

        return position.castle_rights;  // No change
    }

    Square Board::Impl::calculate_new_en_passant(const Move move) {
        const Piece piece = get_piece_at(move.from());
        if (get_piece_type(piece) != PieceType::PAWN)
            return Square::INVALID;

        const int from_rank = (int)move.from() / 8;
        const int to_rank = (int)move.to() / 8;

        if (std::abs(from_rank - to_rank) == 2) {  // Pawn moved 2 squares
            // En passant square is the square between from and to
            return (Square)(((int)move.from() + (int)move.to()) / 2);
        }

        return Square::INVALID;
    }

    void Board::Impl::apply_move(const Move& move) {
        const MoveUndo undo = {
            .move = move,
            .captured_piece = get_piece_at(move.to()),
            .old_castle_rights = position.castle_rights,
            .old_en_passant = position.en_passant_square,
            .old_halfmove_clock = position.halfmove_clock,
            .old_hash = position.zobrist_hash,
        };
        undo_history.push_back(undo);

        const Square from = move.from();
        const Piece piece = get_piece_at(from);
        Color color = get_piece_color(piece);
        PieceType type = get_piece_type(piece);


        const Square to = move.to();
        const Piece captured = get_piece_at(to);
        if (captured != Piece::NONE) {
            Color op_color = get_piece_color(captured);
            PieceType op_type = get_piece_type(captured);
            internal::toggle_bit(position.pieces[(int)op_color][(int)op_type], to);
        }

        internal::toggle_bit(position.pieces[(int)color][(int)type], from);
        internal::toggle_bit(position.pieces[(int)color][(int)type], to);

        if (move.flag() == MoveFlag::PROMOTION) {
            internal::toggle_bit(position.pieces[(int)color][(int)PieceType::PAWN], to);
            internal::toggle_bit(position.pieces[(int)color][(int)move.promotion()], to);
        }

        if (move.flag() == MoveFlag::CASTLING) {
            if (to == Square::G1) {  // White kingside
                internal::toggle_bit(position.pieces[(int)Color::WHITE][(int)PieceType::ROOK], Square::H1);
                internal::toggle_bit(position.pieces[(int)Color::WHITE][(int)PieceType::ROOK], Square::F1);
            } else if (to == Square::C1) {  // White queenside
                internal::toggle_bit(position.pieces[(int)Color::WHITE][(int)PieceType::ROOK], Square::A1);
                internal::toggle_bit(position.pieces[(int)Color::WHITE][(int)PieceType::ROOK], Square::D1);
            } else if (to == Square::G8) {  // Black kingside
                internal::toggle_bit(position.pieces[(int)Color::BLACK][(int)PieceType::ROOK], Square::H8);
                internal::toggle_bit(position.pieces[(int)Color::BLACK][(int)PieceType::ROOK], Square::F8);
            } else if (to == Square::C8) {  // Black queenside
                internal::toggle_bit(position.pieces[(int)Color::BLACK][(int)PieceType::ROOK], Square::A8);
                internal::toggle_bit(position.pieces[(int)Color::BLACK][(int)PieceType::ROOK], Square::D8);
            }
        }

        if (move.flag() == MoveFlag::EN_PASSANT) {
            // Calculate pawn location (one rank back)
            Square captured_pawn_sq = (color == Color::WHITE)
                ? (Square)((int)to - 8)  // One rank down for black pawn
                : (Square)((int)to + 8); // One rank up for white pawn

            // Remove the captured pawn
            Color enemy_color = (color == Color::WHITE) ? Color::BLACK : Color::WHITE;
            internal::toggle_bit(position.pieces[(int)enemy_color][(int)PieceType::PAWN], captured_pawn_sq);
        }

        const uint8_t new_castle = calculate_new_castle_rights(move);
        const Square new_en_passant = calculate_new_en_passant(move);

        position.zobrist_hash = hasher.update(
            position.zobrist_hash,
            move,
            piece,
            captured,
            position.castle_rights,
            new_castle,
            position.en_passant_square,
            new_en_passant
        );

        position.castle_rights = new_castle;
        position.en_passant_square = new_en_passant;
        position.side_to_move = (position.side_to_move == Color::WHITE) ? Color::BLACK : Color::WHITE;

        if (move.flag() == MoveFlag::CAPTURE || move.flag() == MoveFlag::EN_PASSANT || get_piece_type(piece) == PieceType::PAWN)
            position.halfmove_clock = 0;
        else
            position.halfmove_clock++;
        if (position.side_to_move == Color::BLACK) ++position.fullmove_number;

        update_occupancy();
    }

    void Board::Impl::restore_from_history() {
        const MoveUndo undo = undo_history.back();
        const Move move = undo.move;
        undo_history.pop_back();

        const Square from = move.from();
        const Square to = move.to();

        // ✓ Get piece from TO (where it currently is)
        const Piece piece = get_piece_at(to);
        Color color = get_piece_color(piece);
        PieceType type = get_piece_type(piece);

        // === Restore captured piece ===
        if (undo.captured_piece != Piece::NONE) {
            Color op_color = get_piece_color(undo.captured_piece);
            PieceType op_type = get_piece_type(undo.captured_piece);
            internal::toggle_bit(position.pieces[(int)op_color][(int)op_type], to);  // Put it back
        }

        // === Restore piece to source ===
        internal::toggle_bit(position.pieces[(int)color][(int)type], to);    // Remove from to
        internal::toggle_bit(position.pieces[(int)color][(int)type], from);  // Add to from

        // === Restore en passant captured pawn ===
        if (move.flag() == MoveFlag::EN_PASSANT) {
            Square captured_pawn_sq = (color == Color::WHITE)
                ? (Square)((int)to - 8)
                : (Square)((int)to + 8);

            Color enemy_color = (color == Color::WHITE) ? Color::BLACK : Color::WHITE;
            internal::toggle_bit(position.pieces[(int)enemy_color][(int)PieceType::PAWN], captured_pawn_sq);
        }

        // === Handle promotion undo ===
        if (move.flag() == MoveFlag::PROMOTION) {
            internal::toggle_bit(position.pieces[(int)color][(int)move.promotion()], from);  // Remove promoted
            internal::toggle_bit(position.pieces[(int)color][(int)PieceType::PAWN], from);   // Add pawn back
        }

        // === Handle castling undo ===
        if (move.flag() == MoveFlag::CASTLING) {
            if (to == Square::G1) {  // White kingside
                internal::toggle_bit(position.pieces[(int)Color::WHITE][(int)PieceType::ROOK], Square::F1);  // Remove from F1
                internal::toggle_bit(position.pieces[(int)Color::WHITE][(int)PieceType::ROOK], Square::H1);  // Add to H1
            } else if (to == Square::C1) {  // White queenside
                internal::toggle_bit(position.pieces[(int)Color::WHITE][(int)PieceType::ROOK], Square::D1);
                internal::toggle_bit(position.pieces[(int)Color::WHITE][(int)PieceType::ROOK], Square::A1);
            } else if (to == Square::G8) {  // Black kingside
                internal::toggle_bit(position.pieces[(int)Color::BLACK][(int)PieceType::ROOK], Square::F8);
                internal::toggle_bit(position.pieces[(int)Color::BLACK][(int)PieceType::ROOK], Square::H8);
            } else if (to == Square::C8) {  // Black queenside
                internal::toggle_bit(position.pieces[(int)Color::BLACK][(int)PieceType::ROOK], Square::D8);
                internal::toggle_bit(position.pieces[(int)Color::BLACK][(int)PieceType::ROOK], Square::A8);
            }
        }

        position.zobrist_hash = undo.old_hash;
        position.castle_rights = undo.old_castle_rights;
        position.en_passant_square = undo.old_en_passant;
        position.side_to_move = (position.side_to_move == Color::WHITE) ? Color::BLACK : Color::WHITE;
        position.halfmove_clock = undo.old_halfmove_clock;
        if (position.side_to_move == Color::WHITE) --position.fullmove_number;

        update_occupancy();
    }

    Square Board::Impl::find_king(Color color) const
    {
        const Bitboard king_bb = position.pieces[(int)color][(int)PieceType::KING];
        if (king_bb == 0)
            return Square::INVALID;  // No king found (invalid board)

        return (Square)internal::lsb(king_bb);
    }

    void Board::Impl::update_occupancy() {
        position.occupancy[(int)Color::WHITE] = 0;
        position.occupancy[(int)Color::BLACK] = 0;
        position.occupancy_all = 0;

        for (int piece = 0; piece < 6; ++piece) {
            position.occupancy[(int)Color::WHITE] |= position.pieces[(int)Color::WHITE][piece];
            position.occupancy[(int)Color::BLACK] |= position.pieces[(int)Color::BLACK][piece];
        }

        position.occupancy_all = position.occupancy[(int)Color::WHITE] | position.occupancy[(int)Color::BLACK];
    }

    void Board::Impl::parse_board(const std::string_view& board_str, Bitboard pieces[2][6]) {
        // Initialize all bitboards to 0
        for (int i = 0; i < 2; ++i) {
            for (int j = 0; j < 6; ++j) {
                pieces[i][j] = 0;
            }
        }

        int square = 56;  // Start at A8 (square 56, top-left)

        for (const char c : board_str) {
            if (c == '/') {
                // Move to next rank (down 2 ranks, back to a-file)
                square -= 16;
                continue;
            }

            if (std::isdigit(c)) {
                // Skip empty squares
                square += (c - '0');
                continue;
            }

            // It's a piece
            Color color = std::isupper(c) ? Color::WHITE : Color::BLACK;
            const char piece_char = std::tolower(c);

            PieceType piece_type;
            switch (piece_char) {
            case 'p': piece_type = PieceType::PAWN;   break;
            case 'n': piece_type = PieceType::KNIGHT; break;
            case 'b': piece_type = PieceType::BISHOP; break;
            case 'r': piece_type = PieceType::ROOK;   break;
            case 'q': piece_type = PieceType::QUEEN;  break;
            case 'k': piece_type = PieceType::KING;   break;
            default: throw std::logic_error("Unknown piece");
            }

            // Set this bit in the appropriate bitboard
            pieces[(int)color][(int)piece_type] |= (1ULL << square);
            square++;
        }
    }

    void Board::Impl::parse_fen(const std::string& fen) {
        // Example: "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1"

        std::istringstream iss(fen);
        std::string board_part, side_part, castle_part, ep_part, hm_part, fm_part;

        iss >> board_part >> side_part >> castle_part >> ep_part >> hm_part >> fm_part;

        // Parse board (process from rank 8 down to rank 1)
        parse_board(board_part, position.pieces);

        // Parse side to move
        if (side_part.length() != 1)
            throw std::invalid_argument("Invalid turn");
        if (side_part.find('w') != std::string::npos)
            position.side_to_move = Color::WHITE;
        else if (side_part.find('b') != std::string::npos)
            position.side_to_move = Color::BLACK;
        else
            throw std::invalid_argument("Invalid turn");

        // Parse castling rights
        position.castle_rights = 0;
        if (castle_part.find('K') != std::string::npos)
            position.castle_rights |= 0b0001;  // White kingside
        if (castle_part.find('Q') != std::string::npos)
            position.castle_rights |= 0b0010;  // White queenside
        if (castle_part.find('k') != std::string::npos)
            position.castle_rights |= 0b0100;  // Black kingside
        if (castle_part.find('q') != std::string::npos)
            position.castle_rights |= 0b1000;  // Black queenside
        // If castle_part == "-", castle_rights remains 0

        // Parse en passant
        position.en_passant_square = string_to_square(ep_part);

        // Parse halfmove/fullmove clocks
        position.halfmove_clock = std::stoi(hm_part);
        position.fullmove_number = std::stoi(fm_part);

        // Compute zobrist hash
        position.zobrist_hash = hasher.compute(position);

        update_occupancy();
    }

    std::string Board::Impl::board_to_fen() const {
        std::string fen;

        for (int rank = 7; rank >= 0; --rank) {
            int empty_count = 0;

            for (int file = 0; file < 8; ++file) {
                const auto sq = (Square)(file + rank * 8);

                if (const Piece p = get_piece_at(sq); p == Piece::NONE) empty_count++;
                else {
                    if (empty_count > 0) {
                        fen += (char)('0' + empty_count);
                        empty_count = 0;
                    }
                    fen += piece_to_char(p);
                }
            }

            if (empty_count > 0) fen += (char)('0' + empty_count);

            if (rank > 0) fen += '/';
        }

        return fen;
    }

    std::string Board::Impl::position_to_fen() const
    {
        std::string fen = board_to_fen();
        fen += std::format(" {}", position.side_to_move == Color::WHITE ? 'w' : 'b');
        fen += std::format(" {}", castle_rights_to_string(position.castle_rights));
        fen += std::format(" {}", square_to_string(position.en_passant_square));
        fen += std::format(" {}", position.halfmove_clock);
        fen += std::format(" {}", position.fullmove_number);

        return fen;
    }

    void Board::Impl::reset() {
        parse_board(DEFAULT_BOARD, position.pieces);

        position.side_to_move = Color::WHITE;
        position.castle_rights = 0b1111;
        position.en_passant_square = Square::INVALID;
        position.halfmove_clock = 0;
        position.fullmove_number = 1;

        position.zobrist_hash = hasher.compute(position);

        update_occupancy();

        undo_history.clear();
    }

    std::string Board::Impl::castle_rights_to_string(const uint8_t castle_rights) const {
        std::string str;
        if (castle_rights == 0) str += '-';
        else {
            if (castle_rights & 0b0001) str += 'K';
            if (castle_rights & 0b0010) str += 'Q';
            if (castle_rights & 0b0100) str += 'k';
            if (castle_rights & 0b1000) str += 'q';
        }
        return str;
    }

    std::string Board::Impl::board_to_ascii() const
    {
        std::string ascii;

        for (int rank = 7; rank >= 0; --rank) {
            for (int file = 0; file < 8; ++file) {
                const auto sq = (Square)(file + rank * 8);
                const Piece p = get_piece_at(sq);

                ascii += (p == Piece::NONE) ? '.' : piece_to_char(p);
                ascii += ' ';
            }

            ascii += std::to_string(rank + 1);
            ascii += '\n';
        }

        ascii += "a b c d e f g h";
        return ascii;
    }

    Piece Board::Impl::get_piece_at(const Square sq) const {
        for (int color = 0; color < 2; ++color)
            for (int piece_type = 0; piece_type < 6; ++piece_type)
                if (internal::get_bit(position.pieces[color][piece_type], sq))
                    return (Piece)(color * 6 + piece_type);
        return Piece::NONE;
    }

    std::vector<Square> Board::Impl::pieces_of_color(const Color color) const {
        std::vector<Square> squares;

        for (int piece_type = 0; piece_type < 6; ++piece_type) {
            Bitboard bb = position.pieces[(int)color][piece_type];

            while (bb) {
                int sq = internal::lsb(bb);
                squares.push_back((Square)sq);
                bb &= bb - 1;  // Pop LSB
            }
        }

        return squares;
    }

    std::vector<Square> Board::Impl::pieces_of_type(const Color color, PieceType type) const {
        std::vector<Square> squares;
        Bitboard bb = position.pieces[(int)color][(int)type];

        while (bb) {
            int sq = internal::lsb(bb);
            squares.push_back((Square)sq);
            bb &= bb - 1;  // Pop LSB
        }

        return squares;
    }

    // ============================================================================
    // Board Interface
    // ============================================================================

    Board::Board() : impl(std::make_unique<Impl>()) {}

    Board::~Board() = default;

    Board::Board(const Board& board): impl(std::make_unique<Impl>(*board.impl)) {}

    Board& Board::operator=(const Board& other)
    {
        if (this  != &other)
            impl = std::make_unique<Impl>(*other.impl);
        return  *this;
    }

    void Board::load_fen(const std::string& fen) const {
        impl->parse_fen(fen);
    }

    std::string Board::to_fen() const {
        return impl->position_to_fen();
    }

    void Board::reset() const {
        impl->reset();
    }

    Piece Board::piece_at(const Square sq) const {
        return impl->get_piece_at(sq);
    }

    std::vector<Square> Board::pieces_of_type(const Color color, const PieceType type) const {
        return impl->pieces_of_type(color, type);
    }

    std::vector<Square> Board::pieces_of_color(const Color color) const {
        return impl->pieces_of_color(color);
    }

    Color Board::side_to_move() const {
        return impl->position.side_to_move;
    }

    bool Board::can_castle_kingside(const Color color) const{
        const uint8_t mask = (color == Color::WHITE) ? 0b0001 : 0b0100;
        return (impl->position.castle_rights & mask) != 0;
    }

    bool Board::can_castle_queenside(const Color color) const {
        const uint8_t mask = (color == Color::WHITE) ? 0b0010 : 0b1000;
        return (impl->position.castle_rights & mask) != 0;
    }

    Square Board::en_passant_square() const {
        return impl->position.en_passant_square;
    }

    int Board::halfmove_clock() const {
        return impl->position.halfmove_clock;
    }

    int Board::fullmove_number() const {
        return impl->position.fullmove_number;
    }

    void Board::generate_moves(MoveList& moves) const {
        MoveList pseudo_legal;
        impl->generate_pseudo_legal_moves(pseudo_legal);

        moves.clear();

        // Filter each move for legality
        for (const auto& move : pseudo_legal)
            if (is_legal_move(move)) moves.add(move);
    }

    void Board::generate_captures(MoveList& moves) const {
        MoveList legal_moves;
        generate_moves(legal_moves);

        moves.clear();
        for (const Move& move : legal_moves)
            if (move.is_capture()) moves.add(move);
    }

    bool Board::is_legal_move(const Move move) const {
        const Color my_color = side_to_move();
        impl->apply_move(move);
        const bool king_safe = !impl->is_king_under_attack(my_color);
        impl->restore_from_history();
        return king_safe;
    }

    void Board::make_move(const Move move) const {
        if (!is_legal_move(move))
            throw std::invalid_argument("Illegal move");
        impl->apply_move(move);
    }

    void Board::undo_move() const {
        if (impl->undo_history.empty())
            throw std::runtime_error("No moves to undo");
        impl->restore_from_history();
    }

    std::vector<Move> Board::move_history() const {
        return impl->get_move_history();
    }

    void Board::clear_history() const {
        impl->undo_history.clear();
    }

    bool Board::is_in_check() const {
        return impl->is_king_under_attack(side_to_move());
    }

    bool Board::is_checkmate() const {
        if (!is_in_check()) return false;

        MoveList moves;
        generate_moves(moves);
        return moves.empty();
    }

    bool Board::is_stalemate() const {
        if (is_in_check()) return false;

        MoveList moves;
        generate_moves(moves);
        return moves.empty();
    }

    bool Board::is_50_move_draw() const {
        return impl->position.halfmove_clock >= 100;
    }

    int Board::position_repetitions() const {
        return  std::ranges::count_if(impl->undo_history,
             [current_hash = impl->position.zobrist_hash](const MoveUndo& undo) {
                     return undo.old_hash == current_hash;
                 });
    }

    bool Board::is_threefold_repetition() const {
        return position_repetitions() >= 3;
    }

    bool Board::is_game_over() const {
        return is_checkmate() || is_stalemate() || is_50_move_draw() || is_threefold_repetition();
    }

    std::optional<double> Board::game_result() const {
        if (is_checkmate()) // If white is to move and in checkmate, black won
            return (impl->position.side_to_move == Color::WHITE) ? 0.0 : 1.0;

        if (is_stalemate() || is_50_move_draw() || is_threefold_repetition())
            return 0.5;

        return std::nullopt;
    }

    std::string Board::to_string() const {
        return impl->board_to_ascii();
    }

    uint64_t Board::zobrist_hash() const {
        return impl->position.zobrist_hash;
    }

    bool Board::is_valid_position() const {
        // TODO: Implement position validation
        return true;
    }

}  // namespace chess
