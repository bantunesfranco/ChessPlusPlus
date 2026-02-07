# Chess Engine Data Structures

## Core Types & Enums

### Basic Type Aliases
```cpp
using Bitboard = uint64_t;
using Move = uint32_t;  // Can pack all move info into 32 bits
using Score = int16_t;   // Evaluation scores
using Depth = int8_t;    // Search depth
using Hash = uint64_t;   // Zobrist hash
```

### Enumerations
```cpp
enum Color { WHITE = 0, BLACK = 1 };

enum PieceType { PAWN, KNIGHT, BISHOP, ROOK, QUEEN, KING, NO_PIECE = 6 };

enum Piece { 
    WHITE_PAWN, WHITE_KNIGHT, WHITE_BISHOP, WHITE_ROOK, WHITE_QUEEN, WHITE_KING,
    BLACK_PAWN, BLACK_KNIGHT, BLACK_BISHOP, BLACK_ROOK, BLACK_QUEEN, BLACK_KING,
    NO_PIECE_TYPE = 12
};

enum Square {
    A1, B1, C1, D1, E1, F1, G1, H1,
    A2, B2, C2, D2, E2, F2, G2, H2,
    // ... A8-H8
    NO_SQUARE = 64
};

enum MoveType { 
    NORMAL, CAPTURE, PROMOTION, CASTLING, EN_PASSANT 
};

enum CastleRights { 
    NO_CASTLE = 0,
    WHITE_KINGSIDE = 1,
    WHITE_QUEENSIDE = 2,
    BLACK_KINGSIDE = 4,
    BLACK_QUEENSIDE = 8
};
```

---

## Board Position Structure

### Position (Primary Board Representation)
```cpp
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
```

**Rationale**: 
- Separate bitboards for each piece type enable fast, targeted queries
- Occupancy bitboards cached for quick collision detection
- Side-to-move stored explicitly (avoids confusion)
- Zobrist hash pre-computed for transposition table efficiency

---

## Move Representation

### Compact Move Encoding (32-bit)
```cpp
// Move layout: [promotion: 2 bits][move_type: 3 bits][to_square: 6 bits][from_square: 6 bits]
class Move {
public:
    uint32_t data;
    
    Move() : data(0) {}
    
    Move(Square from, Square to, MoveType type = NORMAL, int promotion = 0) {
        data = (from) | (to << 6) | (type << 12) | (promotion << 15);
    }
    
    Square from_square() const { return Square(data & 0x3F); }
    Square to_square() const { return Square((data >> 6) & 0x3F); }
    MoveType move_type() const { return MoveType((data >> 12) & 0x7); }
    int promotion_piece() const { return (data >> 15) & 0x3; }  // KNIGHT, BISHOP, ROOK, QUEEN
    
    bool is_capture() const { return move_type() == CAPTURE; }
    bool is_promotion() const { return move_type() == PROMOTION; }
    bool is_castling() const { return move_type() == CASTLING; }
    bool is_en_passant() const { return move_type() == EN_PASSANT; }
    
    bool operator==(const Move& other) const { return data == other.data; }
    bool operator!=(const Move& other) const { return data != other.data; }
};
```

**Rationale**: 
- All move information (source, destination, type, promotion) in single 32-bit value
- Fast comparison and storage
- Enables efficient move lists

---

## Move List & Generation

### Move List (Dynamic Array)
```cpp
class MoveList {
private:
    std::array<Move, 256> moves;  // Max ~218 legal moves in chess
    size_t count = 0;
    
public:
    void add(Move m) { moves[count++] = m; }
    void clear() { count = 0; }
    
    Move operator[](size_t idx) const { return moves[idx]; }
    size_t size() const { return count; }
    
    auto begin() { return moves.begin(); }
    auto end() { return moves.begin() + count; }
};
```

**Rationale**:
- Static allocation avoids dynamic memory in tight loops
- 256 size is well above actual maximum (~218 moves)
- Efficient iteration for move ordering and search

---

## Evaluation & Search

### Evaluation Score
```cpp
// Special score values
constexpr Score CHECKMATE_SCORE = 32700;
constexpr Score STALEMATE_SCORE = 0;
constexpr Score ILLEGAL_SCORE = -32768;

// Mate detection helpers
inline bool is_mate(Score s) { 
    return std::abs(s) >= CHECKMATE_SCORE - 100; 
}

inline int mate_distance(Score s) {
    return (CHECKMATE_SCORE - std::abs(s)) / 2;
}
```

### Transposition Table Entry
```cpp
enum Flag { EXACT, LOWER_BOUND, UPPER_BOUND };

struct TTEntry {
    Hash key;           // 64-bit Zobrist hash for position verification
    Score score;        // Evaluation score
    Depth depth;        // Search depth this entry was calculated at
    uint8_t flag;       // EXACT, LOWER_BOUND, or UPPER_BOUND
    Move best_move;     // Best move found at this position
    
    bool matches(Hash h, Depth d) const {
        return key == h && depth >= d;
    }
};
```

### Transposition Table (Hash Table)
```cpp
class TranspositionTable {
private:
    std::vector<TTEntry> table;
    size_t mask;
    
public:
    TranspositionTable(size_t mb_size) {
        size_t entry_count = (mb_size * 1024 * 1024) / sizeof(TTEntry);
        // Round down to nearest power of 2
        entry_count = 1LL << (63 - __builtin_clzll(entry_count));
        table.resize(entry_count);
        mask = entry_count - 1;
    }
    
    void store(Hash h, Score s, Depth d, uint8_t f, Move m) {
        TTEntry& entry = table[h & mask];
        entry = {h, s, d, f, m};
    }
    
    bool lookup(Hash h, Depth d, Score& out_score, Move& out_move) {
        const TTEntry& entry = table[h & mask];
        if (entry.key == h && entry.depth >= d) {
            out_score = entry.score;
            out_move = entry.best_move;
            return true;
        }
        return false;
    }
    
    void clear() { std::fill(table.begin(), table.end(), TTEntry{}); }
    size_t size_mb() const { return (table.size() * sizeof(TTEntry)) / (1024 * 1024); }
};
```

**Rationale**:
- Hash table enables memoization of evaluated positions
- Mask-based indexing (power of 2 size) is faster than modulo
- Flag system handles alpha-beta bounds
- Always stores, replacing weaker entries

---

## Move Ordering & Selection

### Killer Moves Array
```cpp
class KillerMoves {
private:
    std::array<std::array<Move, 2>, MAX_DEPTH> killers;  // 2 killers per depth
    
public:
    void store(Depth d, Move m) {
        if (d < MAX_DEPTH && killers[d][0] != m) {
            killers[d][1] = killers[d][0];
            killers[d][0] = m;
        }
    }
    
    bool is_killer(Depth d, Move m) const {
        if (d >= MAX_DEPTH) return false;
        return killers[d][0] == m || killers[d][1] == m;
    }
    
    void clear() { 
        for (auto& row : killers) row.fill(Move()); 
    }
};
```

### Move History Array (History Heuristic)
```cpp
class HistoryHeuristic {
private:
    std::array<std::array<int, 64>, 64> history;  // [from][to]
    
public:
    void store(Square from, Square to, Depth d) {
        history[from][to] += d * d;  // Bonus proportional to depth
    }
    
    int get_score(Square from, Square to) const {
        return history[from][to];
    }
    
    void clear() { 
        for (auto& row : history) row.fill(0); 
    }
};
```

### Move Picker (For Iterative Move Selection)
```cpp
class MovePicker {
private:
    MoveList moves;
    std::vector<int> scores;
    size_t index = 0;
    
public:
    MovePicker(const Position& pos, const KillerMoves& killers, 
               const HistoryHeuristic& history) {
        // Generate moves and score them
        generate_moves(pos, moves);
        scores.resize(moves.size());
        score_moves(pos, moves, killers, history, scores);
    }
    
    bool next_move(Move& out_move) {
        if (index >= moves.size()) return false;
        
        // Find best unselected move
        int best_idx = index;
        for (size_t i = index + 1; i < moves.size(); ++i) {
            if (scores[i] > scores[best_idx]) best_idx = i;
        }
        
        // Swap to current position and return
        std::swap(moves[index], moves[best_idx]);
        std::swap(scores[index], scores[best_idx]);
        out_move = moves[index++];
        return true;
    }
};
```

---

## Search State & Context

### Search Stack Entry
```cpp
struct SearchStackEntry {
    Move current_move;
    Move killer_moves[2];
    int static_eval;
    bool in_check;
};

using SearchStack = std::array<SearchStackEntry, MAX_DEPTH>;
```

### Search Info (Statistics & Time Management)
```cpp
struct SearchInfo {
    uint64_t nodes_searched = 0;
    uint64_t quiesce_nodes = 0;
    uint64_t tt_hits = 0;
    uint64_t tt_cutoffs = 0;
    
    std::chrono::steady_clock::time_point start_time;
    std::chrono::milliseconds time_limit;
    
    bool time_expired() const {
        auto elapsed = std::chrono::steady_clock::now() - start_time;
        return elapsed > time_limit;
    }
    
    void reset() {
        nodes_searched = 0;
        quiesce_nodes = 0;
        tt_hits = 0;
        tt_cutoffs = 0;
    }
};
```

---

## Game State & History

### Move History Entry (For Undo)
```cpp
struct MoveUndo {
    Move move;
    Piece captured_piece;
    uint8_t old_castle_rights;
    Square old_en_passant;
    uint16_t old_halfmove_clock;
    Hash old_hash;
};
```

### Game State Container
```cpp
class GameState {
private:
    std::vector<Position> position_history;
    std::vector<MoveUndo> move_history;
    
public:
    void push_move(const Position& pos, Move m, const MoveUndo& undo) {
        position_history.push_back(pos);
        move_history.push_back(undo);
    }
    
    void pop_move() {
        if (!position_history.empty()) {
            position_history.pop_back();
            move_history.pop_back();
        }
    }
    
    int repetition_count(const Position& current_pos) const {
        int count = 0;
        for (const auto& prev_pos : position_history) {
            if (prev_pos.zobrist_hash == current_pos.zobrist_hash) {
                count++;
            }
        }
        return count;
    }
};
```

---

## Zobrist Hashing

### Zobrist Hash Generator
```cpp
class ZobristHasher {
private:
    std::array<std::array<Hash, 64>, 12> piece_hashes;  // [piece][square]
    std::array<Hash, 16> castle_hashes;                  // [castle_rights]
    std::array<Hash, 8> en_passant_hashes;               // [en_passant_file] (no hash if none)
    Hash black_move_hash;
    
public:
    ZobristHasher() {
        // Initialize with pseudo-random values (seed with fixed constant)
        std::mt19937_64 rng(0x123456789ABCDEFULL);
        std::uniform_int_distribution<Hash> dist;
        
        for (auto& row : piece_hashes) {
            for (auto& h : row) h = dist(rng);
        }
        for (auto& h : castle_hashes) h = dist(rng);
        for (auto& h : en_passant_hashes) h = dist(rng);
        black_move_hash = dist(rng);
    }
    
    Hash compute(const Position& pos) {
        Hash h = 0;
        
        // Hash pieces
        for (int color = 0; color < 2; ++color) {
            for (int piece = 0; piece < 6; ++piece) {
                Bitboard bb = pos.pieces[color][piece];
                while (bb) {
                    int sq = lsb(bb);
                    h ^= piece_hashes[color * 6 + piece][sq];
                    bb &= bb - 1;  // Clear LSB
                }
            }
        }
        
        // Hash castle rights
        h ^= castle_hashes[pos.castle_rights];
        
        // Hash en passant
        if (pos.en_passant_square != NO_SQUARE) {
            h ^= en_passant_hashes[pos.en_passant_square % 8];
        }
        
        // Hash side to move
        if (pos.side_to_move == BLACK) h ^= black_move_hash;
        
        return h;
    }
    
    Hash update(Hash h, const Position& old_pos, const Position& new_pos) {
        // Incremental updates would go here for performance
        return compute(new_pos);
    }
};
```

---

## Piece-Square Tables

### Evaluation Tables
```cpp
class PieceSquareTables {
private:
    // Tables for each piece type (endgame and midgame)
    std::array<std::array<int, 64>, 6> midgame_tables;
    std::array<std::array<int, 64>, 6> endgame_tables;
    
public:
    PieceSquareTables() {
        // Initialize with evaluation bonuses
        // Example: center squares, king safety, piece activity, etc.
        // Tables indexed by [piece_type][square]
    }
    
    int get_midgame_value(PieceType pt, Square sq, Color c) const {
        return c == WHITE ? midgame_tables[pt][sq] : midgame_tables[pt][63 - sq];
    }
    
    int get_endgame_value(PieceType pt, Square sq, Color c) const {
        return c == WHITE ? endgame_tables[pt][sq] : endgame_tables[pt][63 - sq];
    }
};
```

---

## Best Practices Summary

| Structure | Purpose | Key Feature |
|-----------|---------|------------|
| **Position** | Board state | 12 bitboards + cached occupancy |
| **Move** | Packed move data | 32-bit encoding (all info in one int) |
| **MoveList** | Move storage | Static allocation, no heap pressure |
| **TranspositionTable** | Position memoization | Hash table with depth-based replacement |
| **KillerMoves** | Move ordering hint | Fast lookup for beta-cutoff moves |
| **HistoryHeuristic** | Move ordering hint | Tracks frequently good moves |
| **SearchStack** | Search context | Per-depth state for recursion |
| **ZobristHasher** | Hash computation | Fast position fingerprinting |

---

## Memory Layout Tips

- Align bitboards on 8-byte boundaries
- Group frequently-accessed fields together in Position struct
- Use static allocation for move lists (stack vs heap is faster)
- Transposition table size: 16-32 MB for casual play, 64-512 MB for serious play
- Consider cache line alignment (64 bytes) for performance-critical structures

