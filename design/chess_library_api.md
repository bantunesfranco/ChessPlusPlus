# Chess Engine Library - Public API Design

## Overview

Design a clean, minimal public API that exposes core chess functionality while hiding implementation details. This allows users to:
- Create and manipulate board positions
- Generate legal moves
- Search for best moves
- Evaluate positions
- Play games programmatically

---

## API Organization

### Header Structure
```
include/
├── chess/
│   ├── engine.hpp          # Main entry point
│   ├── board.hpp           # Board representation
│   ├── move.hpp            # Move types & generation
│   ├── search.hpp          # Search interface
│   └── eval.hpp            # Evaluation interface
└── chess.hpp               # Single include (includes all above)

src/
├── board.cpp
├── move_gen.cpp
├── search.cpp
├── eval.cpp
└── ... (internal implementation details)
```

### Public vs. Private Separation
```
Public API (in chess/ headers):
- Basic types: Square, Color, Piece, Move
- Board class: Position management
- Engine class: High-level interface
- Move generation: Legal move queries
- Search: Best move finding
- Evaluation: Position scoring

Internal (not exposed):
- Bitboard utilities
- Zobrist hashing internals
- Transposition table details
- Move ordering heuristics
- Piece-square table data
- Search implementation details
```

---

## Core Type Definitions

### Header: `chess/types.hpp`
```cpp
#pragma once

namespace chess {

// Basic enumerations
enum class Color : uint8_t { WHITE = 0, BLACK = 1 };

enum class Square : uint8_t {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    A3, B3, C3, D3, E3, F3, G3, H3,
    A4, B4, C4, D4, E4, F4, G4, H4,
    A5, B5, C5, D5, E5, F5, G5, H5,
    A6, B6, C6, D6, E6, F6, G6, H6,
    A7, B7, C7, D7, E7, F7, G7, H7,
    A8, B8, C8, D8, E8, F8, G8, H8,
    INVALID = 64
};

enum class PieceType : uint8_t { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NONE };

enum class Piece : uint8_t {
    WHITE_PAWN, WHITE_KNIGHT, WHITE_BISHOP, WHITE_ROOK, WHITE_QUEEN, WHITE_KING,
    BLACK_PAWN, BLACK_KNIGHT, BLACK_BISHOP, BLACK_ROOK, BLACK_QUEEN, BLACK_KING,
    NONE = 12
};

// Score type (evaluation in centipawns)
using Score = int16_t;
constexpr Score CHECKMATE = 32700;
constexpr Score STALEMATE = 0;

// Move flags
enum class MoveFlag : uint8_t {
    NORMAL = 0,
    CAPTURE = 1,
    PROMOTION = 2,
    CASTLING = 3,
    EN_PASSANT = 4
};

// Utility functions for Square
inline int square_file(Square sq) { return static_cast<int>(sq) % 8; }
inline int square_rank(Square sq) { return static_cast<int>(sq) / 8; }
inline Square make_square(int file, int rank) { 
    return Square(file + rank * 8); 
}

std::string square_to_string(Square sq);
Square string_to_square(const std::string& s);

}  // namespace chess
```

---

## Move Representation

### Header: `chess/move.hpp`
```cpp
#pragma once

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
    Square from() const { return Square(data & 0x3F); }
    Square to() const { return Square((data >> 6) & 0x3F); }
    MoveFlag flag() const { return MoveFlag((data >> 12) & 0x7); }
    PieceType promotion() const { return PieceType((data >> 15) & 0x7); }

    // Query methods
    bool is_capture() const { return flag() == MoveFlag::CAPTURE; }
    bool is_promotion() const { return flag() == MoveFlag::PROMOTION; }
    bool is_castling() const { return flag() == MoveFlag::CASTLING; }
    bool is_en_passant() const { return flag() == MoveFlag::EN_PASSANT; }

    // Comparison
    bool operator==(const Move& other) const { return data == other.data; }
    bool operator!=(const Move& other) const { return data != other.data; }

    // Conversion to/from algebraic notation
    std::string to_uci() const;
    static Move from_uci(const std::string& uci_str);
    
    uint32_t raw() const { return data; }
};

// Move list for generator
class MoveList {
private:
    std::array<Move, 256> moves;
    size_t count = 0;

public:
    void add(Move m) { moves[count++] = m; }
    void clear() { count = 0; }

    size_t size() const { return count; }
    bool empty() const { return count == 0; }
    
    Move operator[](size_t idx) const { return moves[idx]; }
    Move at(size_t idx) const {
        if (idx >= count) throw std::out_of_range("MoveList index out of range");
        return moves[idx];
    }

    // Iterator support
    auto begin() { return moves.begin(); }
    auto end() { return moves.begin() + count; }
    auto begin() const { return moves.begin(); }
    auto end() const { return moves.begin() + count; }
};

}  // namespace chess
```

---

## Board State Management

### Header: `chess/board.hpp`
```cpp
#pragma once

#include "types.hpp"
#include "move.hpp"

namespace chess {

class Board {
public:
    Board();
    
    // === Initialization ===
    
    /// Load position from FEN string
    /// @param fen Forsyth-Edwards Notation string
    /// @throws std::invalid_argument if FEN is malformed
    void load_fen(const std::string& fen);
    
    /// Get current position as FEN
    /// @return FEN string representation
    std::string to_fen() const;
    
    /// Reset to standard starting position
    void reset();

    // === Board Queries ===
    
    /// Get piece at square
    Piece piece_at(Square sq) const;
    
    /// Get all pieces of type for color
    std::vector<Square> pieces_of_type(Color color, PieceType type) const;
    
    /// Get all pieces for a color
    std::vector<Square> pieces_of_color(Color color) const;
    
    /// Whose turn is it?
    Color side_to_move() const { return side; }
    
    /// Can this color castle kingside?
    bool can_castle_kingside(Color color) const;
    
    /// Can this color castle queenside?
    bool can_castle_queenside(Color color) const;
    
    /// Get en passant target square (or INVALID if none)
    Square en_passant_square() const;
    
    /// Get halfmove clock (for 50-move rule)
    int halfmove_clock() const;
    
    /// Get fullmove number
    int fullmove_number() const;

    // === Move Handling ===
    
    /// Generate all legal moves
    /// @param moves MoveList to fill with legal moves
    void generate_moves(MoveList& moves) const;
    
    /// Generate only capture moves
    void generate_captures(MoveList& moves) const;
    
    /// Check if a move is legal
    bool is_legal_move(Move move) const;
    
    /// Make a move (modifies board state)
    /// @throws std::invalid_argument if move is illegal
    void make_move(Move move);
    
    /// Undo the last move
    /// @throws std::runtime_error if no moves to undo
    void undo_move();
    
    /// Get move history
    std::vector<Move> move_history() const;
    
    /// Clear move history
    void clear_history();

    // === Game State Queries ===
    
    /// Is the current side in check?
    bool is_in_check() const;
    
    /// Is current position checkmate?
    bool is_checkmate() const;
    
    /// Is current position stalemate?
    bool is_stalemate() const;
    
    /// Has game reached 50-move rule draw?
    bool is_50_move_draw() const;
    
    /// Count repetitions of current position in history
    int position_repetitions() const;
    
    /// Is current position a threefold repetition?
    bool is_threefold_repetition() const;
    
    /// Is game over? (checkmate, stalemate, draws)
    bool is_game_over() const;
    
    /// Get game result: 1.0 (white win), 0.5 (draw), 0.0 (black win), none if ongoing
    std::optional<double> game_result() const;

    // === Display ===
    
    /// ASCII representation of board
    std::string to_string() const;

    // === Advanced Queries ===
    
    /// Get piece square hash (for transposition table, if needed)
    uint64_t zobrist_hash() const;
    
    /// Check if position is legal (no double checks, etc)
    bool is_valid_position() const;

private:
    // Internal state (opaque to user)
    class Impl;
    std::unique_ptr<Impl> impl;
};

}  // namespace chess
```

---

## Move Generation Interface

### Header: `chess/move_gen.hpp`
```cpp
#pragma once

#include "types.hpp"
#include "move.hpp"
#include "board.hpp"

namespace chess {

class MoveGenerator {
public:
    MoveGenerator() = default;

    /// Generate all legal moves for board position
    static void generate_all_moves(const Board& board, MoveList& moves) {
        board.generate_moves(moves);
    }
    
    /// Generate only captures and promotions
    static void generate_tactical_moves(const Board& board, MoveList& moves) {
        board.generate_captures(moves);
    }
    
    /// Check if a specific move is legal
    static bool is_legal(const Board& board, Move move) {
        return board.is_legal_move(move);
    }
    
    /// Filter moves: keep only those matching predicate
    static void filter_moves(MoveList& moves, 
                           std::function<bool(Move)> predicate) {
        MoveList filtered;
        for (size_t i = 0; i < moves.size(); ++i) {
            if (predicate(moves[i])) {
                filtered.add(moves[i]);
            }
        }
        moves = filtered;
    }
};

}  // namespace chess
```

---

## Position Evaluation

### Header: `chess/eval.hpp`
```cpp
#pragma once

#include "types.hpp"
#include "board.hpp"

namespace chess {

class Evaluator {
public:
    Evaluator();
    
    /// Evaluate position from perspective of side to move
    /// Positive = side to move winning, Negative = side to move losing
    /// @param board Position to evaluate
    /// @return Score in centipawns
    Score evaluate(const Board& board);
    
    /// Evaluate from white's perspective (always)
    /// @return Score in centipawns (positive = white winning)
    Score evaluate_white(const Board& board);
    
    /// Get material balance (in centipawns)
    /// Simple piece count without positional factors
    Score material_count(const Board& board);
    
    /// Estimate phase: 0.0 (endgame) to 1.0 (midgame opening)
    double get_phase(const Board& board);

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

}  // namespace chess
```

---

## Search & Engine

### Header: `chess/search.hpp`
```cpp
#pragma once

#include "types.hpp"
#include "move.hpp"
#include "board.hpp"
#include <chrono>
#include <optional>

namespace chess {

// Search result
struct SearchResult {
    Move best_move;           // Best move found
    Score score;              // Evaluation of position after best move
    int depth;                // Depth searched
    uint64_t nodes_searched;  // Nodes examined
    double search_time;       // Time spent (seconds)
};

// Search configuration
struct SearchConfig {
    // Time control
    std::chrono::milliseconds time_limit = std::chrono::milliseconds(5000);
    
    // Depth limit
    int max_depth = 20;
    
    // Transposition table size in MB
    int tt_size_mb = 64;
    
    // Enable/disable features
    bool use_transposition_table = true;
    bool use_quiescence_search = true;
    bool use_move_ordering = true;
    
    // Callbacks
    std::function<void(const SearchResult&)> on_iteration_complete;
};

class Engine {
public:
    Engine();
    explicit Engine(const SearchConfig& config);
    
    // === Configuration ===
    
    /// Update search configuration
    void set_config(const SearchConfig& config);
    
    /// Get current configuration
    SearchConfig get_config() const;
    
    /// Set transposition table size
    void set_tt_size(int mb);
    
    /// Clear transposition table cache
    void clear_cache();

    // === Main Search Interface ===
    
    /// Find best move with time limit
    /// @param board Position to search
    /// @param time_limit Milliseconds to think
    /// @return Best move and statistics
    SearchResult find_best_move(const Board& board,
                                std::chrono::milliseconds time_limit);
    
    /// Find best move with depth limit
    SearchResult find_best_move_depth(const Board& board, int max_depth);
    
    /// Find best move with both limits (whichever reached first)
    SearchResult find_best_move(const Board& board, int max_depth,
                                std::chrono::milliseconds time_limit);
    
    /// Evaluate position
    Score evaluate(const Board& board);

    // === Advanced Search ===
    
    /// Get principal variation (best line of play)
    std::vector<Move> get_principal_variation(const Board& board, int depth);
    
    /// Get all candidate moves ranked by search preference
    MoveList get_ranked_moves(const Board& board);
    
    /// Analyze position in detail
    struct Analysis {
        Move best_move;
        std::vector<Move> pv;  // Principal variation
        Score score;
        int depth;
        std::vector<std::pair<Move, Score>> move_scores;  // All moves with scores
    };
    
    Analysis analyze(const Board& board, int depth);

    // === Callback/Monitoring ===
    
    /// Register callback for search progress
    void set_progress_callback(
        std::function<void(int depth, uint64_t nodes)> callback);
    
    /// Stop current search
    void stop_search();

private:
    class Impl;
    std::unique_ptr<Impl> impl;
};

}  // namespace chess
```

---

## Main Include File

### Header: `chess.hpp`
```cpp
#pragma once

// Single header to include entire chess library
#include "chess/types.hpp"
#include "chess/move.hpp"
#include "chess/board.hpp"
#include "chess/move_gen.hpp"
#include "chess/eval.hpp"
#include "chess/search.hpp"

// Convenience namespace alias
namespace ch = chess;
```

---

## Usage Examples

### Basic Usage
```cpp
#include <chess.hpp>

int main() {
    using namespace chess;
    
    // Create board
    Board board;
    board.reset();  // Starting position
    
    // Make a move
    board.make_move(Move(Square::E2, Square::E4));
    
    // Print board
    std::cout << board.to_string() << std::endl;
    
    // Get legal moves
    MoveList moves;
    board.generate_moves(moves);
    std::cout << "Legal moves: " << moves.size() << std::endl;
    
    return 0;
}
```

### Engine Search
```cpp
#include <chess.hpp>

int main() {
    using namespace chess;
    
    Board board;
    board.load_fen("rnbqkbnr/pppppppp/8/8/4P3/8/PPPP1PPP/RNBQKBNR b KQkq e3 0 1");
    
    Engine engine;
    SearchConfig config;
    config.time_limit = std::chrono::milliseconds(5000);
    config.max_depth = 20;
    engine.set_config(config);
    
    auto result = engine.find_best_move(board, std::chrono::milliseconds(5000));
    
    std::cout << "Best move: " << result.best_move.to_uci() << std::endl;
    std::cout << "Score: " << result.score << " cp" << std::endl;
    std::cout << "Depth: " << result.depth << std::endl;
    std::cout << "Nodes: " << result.nodes_searched << std::endl;
    std::cout << "Time: " << result.search_time << " s" << std::endl;
    
    return 0;
}
```

### Position Analysis
```cpp
#include <chess.hpp>

int main() {
    using namespace chess;
    
    Board board;
    board.load_fen("r1bqkb1r/pppppppp/2n2n2/8/4P3/5N2/PPPP1PPP/RNBQKB1R w KQkq - 4 4");
    
    Engine engine;
    auto analysis = engine.analyze(board, 20);
    
    std::cout << "Best: " << analysis.best_move.to_uci() << std::endl;
    std::cout << "Score: " << analysis.score << " cp" << std::endl;
    std::cout << "PV: ";
    for (const auto& move : analysis.pv) {
        std::cout << move.to_uci() << " ";
    }
    std::cout << std::endl;
    
    return 0;
}
```

---

## Design Principles

### 1. **Simplicity First**
- Public API minimal and intuitive
- Complex details hidden in `Impl` pointers
- No template hell or header bloat

### 2. **Clear Intent**
- Method names explicit (`is_legal_move` not `valid()`)
- Return types obvious (`std::optional` for optional values)
- Strong types (`Square`, `Color` enums not ints)

### 3. **Error Handling**
```cpp
// Prefer exceptions for error conditions
try {
    board.make_move(illegal_move);  // Throws std::invalid_argument
} catch (const std::exception& e) {
    std::cerr << "Error: " << e.what() << std::endl;
}

// Or check first
if (board.is_legal_move(move)) {
    board.make_move(move);
}
```

### 4. **Performance Options**
```cpp
// Users can configure engine behavior
SearchConfig config;
config.use_transposition_table = false;    // Lighter weight
config.tt_size_mb = 256;                   // More memory = stronger
config.max_depth = 10;                     // Shallow search = faster
```

### 5. **Iterator/Range Support**
```cpp
MoveList moves;
board.generate_moves(moves);

// C++11 range-based for loop
for (Move move : moves) {
    std::cout << move.to_uci() << std::endl;
}

// STL algorithms
std::vector<Move> captures;
std::copy_if(moves.begin(), moves.end(),
             std::back_inserter(captures),
             [](Move m) { return m.is_capture(); });
```

---

## Compilation & CMake

### CMakeLists.txt

```cmake
cmake_minimum_required(VERSION 3.15)
project(chess-engine VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Library target
add_library(chess-engine
        ../src/Board.cpp
        src/move_gen.cpp
        ../src/Search.cpp
        ../src/Eval.cpp
        # ... other sources
)

target_include_directories(chess-engine
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Optional: Example executable
add_executable(chess-demo examples/main.cpp)
target_link_libraries(chess-demo chess-engine)

# Optional: Unit tests
enable_testing()
add_subdirectory(../tests)
```

### Usage as External Library
```cmake
# In external project
find_package(chess-engine 1.0 REQUIRED)

add_executable(my_app main.cpp)
target_link_libraries(my_app chess-engine::chess-engine)
```

---

## Future Extensions (Keep API Stable!)

When adding features, maintain backward compatibility:

```cpp
// Good: Adds new functionality without breaking existing code
namespace chess {
    class Board {
        // ... existing methods ...
        
        // New in v1.1.0
        bool is_insufficient_material() const;
    };
}

// Bad: Would break existing code
class Board {
    // Removing or changing existing methods!
};
```

---

## Summary: Public API Checklist

✅ Board representation and FEN I/O
✅ Legal move generation
✅ Move validation and execution (with undo)
✅ Game state queries (check, mate, draws)
✅ Position evaluation
✅ Search and best move finding
✅ Configurable search parameters
✅ Time/depth control
✅ Principal variation extraction
✅ Analysis interface
✅ Clear error handling
✅ Iterator support for results
✅ No leaky abstractions (bitboards hidden)
✅ CMake integration

