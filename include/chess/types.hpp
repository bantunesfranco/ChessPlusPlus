#pragma once

#include <array>
#include <string>
#include <cstdint>
#include <unordered_map>

namespace chess
{
    enum Color : uint8_t { WHITE = 0, BLACK = 1 };
    static constexpr inline std::array COLORS = {WHITE, BLACK};
    static constexpr inline std::array<std::string, 2> COLOR_NAMES = {"white", "black"};

    enum Square : uint8_t {
        A1, B1, C1, D1, E1, F1, G1, H1,
        A2, B2, C2, D2, E2, F2, G2, H2,
        A3, B3, C3, D3, E3, F3, G3, H3,
        A4, B4, C4, D4, E4, F4, G4, H4,
        A5, B5, C5, D5, E5, F5, G5, H5,
        A6, B6, C6, D6, E6, F6, G6, H6,
        A7, B7, C7, D7, E7, F7, G7, H7,
        A8, B8, C8, D8, E8, F8, G8, H8,
    };

    static constexpr inline std::array SQUARES = {
        A1, B1, C1, D1, E1, F1, G1, H1,
        A2, B2, C2, D2, E2, F2, G2, H2,
        A3, B3, C3, D3, E3, F3, G3, H3,
        A4, B4, C4, D4, E4, F4, G4, H4,
        A5, B5, C5, D5, E5, F5, G5, H5,
        A6, B6, C6, D6, E6, F6, G6, H6,
        A7, B7, C7, D7, E7, F7, G7, H7,
        A8, B8, C8, D8, E8, F8, G8, H8,
    };

    static constexpr inline std::array<std::string, 64> SQUARE_NAMES = {
        "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
        "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
        "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
        "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
        "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
        "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
        "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",
        "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
    };

    enum File : uint8_t { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H };
    static constexpr inline std::array FILES = { FILE_A, FILE_B, FILE_C, FILE_D, FILE_E, FILE_F, FILE_G, FILE_H };
    static constexpr inline std::array<std::string, 8> FILE_NAMES = {"a", "b", "c", "d", "e", "f", "g", "h"};

    enum Rank : uint8_t { RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8 };

    static constexpr inline std::array RANKS = {RANK_1, RANK_2, RANK_3, RANK_4, RANK_5, RANK_6, RANK_7, RANK_8};
    static constexpr inline std::array<std::string, 8> RANK_NAMES = {"1", "2", "3", "4", "5", "6", "7", "8"};

    enum PieceType : uint8_t { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NONE };
    static constexpr inline std::array PIECE_TYPES = {PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING};
    static constexpr inline std::array<std::string, 7> PIECE_SYMBOLS = {"", "p", "n", "b", "r", "q", "k"};
    static constexpr inline std::array<std::string, 7> PIECE_NAMES = {"", "pawn", "knight", "bishop", "rook", "queen", "king"};
    static const inline std::unordered_map<std::string, std::string> UNICODE_PIECE_SYMBOLS = {
        {"R", "♖"}, {"r", "♜"},
        {"N", "♘"}, {"n", "♞"},
        {"B", "♗"}, {"b", "♝"},
        {"Q", "♕"}, {"q", "♛"},
        {"K", "♔"}, {"k", "♚"},
        {"P", "♙"}, {"p", "♟"},
   };

    /*The FEN for the standard chess starting position.*/
    static const std::string STARTING_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    /*The board part of the FEN for the standard chess starting position.*/
    static const std::string STARTING_BOARD_FEN = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR";

    using Bitboard = uint64_t;  // 64-bit bitboard
    using Depth    = int8_t;    // Search depth
    using Hash     = uint64_t;  // Zobrist hash
    using Score    = int32_t;   // Evaluation score in centipawns

    // Special score values
    constexpr Score CHECKMATE = 32700;
    constexpr Score STALEMATE = 0;
    constexpr Score ILLEGAL_SCORE = -32768;

    static constexpr std::array<double, 6> PIECE_VALUES = { 100, 320, 330, 500, 900, 0 };

    // Move flags
    enum class MoveFlag : uint8_t {
        NORMAL = 0,
        CAPTURE = 1,
        PROMOTION = 2,
        CASTLING = 3,
        EN_PASSANT = 4
    };

    enum class CastleRights : uint8_t {
        NO_CASTLE = 0,
        WHITE_KINGSIDE = 1,
        WHITE_QUEENSIDE = 2,
        BLACK_KINGSIDE = 4,
        BLACK_QUEENSIDE = 8
    };

    struct Position {
        // 12 bitboards: 6 piece types × 2 colors
        Bitboard pieces[2][6];  // [color][piece_type]

        // Convenience bitboards
        Bitboard occupancy[2];      // All pieces per color
        Bitboard occupancy_all;     // All pieces on board

        // Game state
        Color side_to_move;
        uint8_t castle_rights;      // Bitmask (4 bits for 4 castling rights)
        Square en_passant_square;   // NO_SQUARE if none
        uint16_t halfmove_clock;    // For 50-move rule
        uint32_t fullmove_number;   // Starting at 1, increments after black move

        // Zobrist hash for transposition table lookups
        Hash zobrist_hash;
    };

    // Utility functions for Square
    inline int square_file(Square sq) { return static_cast<int>(sq) % 8; }
    inline int square_rank(Square sq) { return static_cast<int>(sq) / 8; }
    inline Square make_square(const int file, const int rank) { return Square(file + rank * 8); }

    // Mate detection helpers
    inline bool is_mate(const Score s) { return std::abs(s) >= CHECKMATE - 100; }
    inline int mate_distance(const Score s) { return (CHECKMATE - std::abs(s)) / 2;}

    std::string square_to_string(Square sq);
    Square string_to_square(const std::string& s);
    char piece_to_char(Piece p);
    PieceType get_piece_type(Piece p);
    Color get_piece_color(Piece p);

}  // namespace chess
