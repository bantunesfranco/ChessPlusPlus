# Piece-Square Tables & Positional Incentives

## Overview

Piece-square tables (PSTs) assign bonuses or penalties to pieces based on their position on the board. This allows the engine to prefer certain square placements (e.g., centralizing knights, keeping kings safe during middlegame, activating them in endgame).

---

## Basic Concept

Each square receives a value bonus/penalty per piece type. Rather than just counting material, the engine evaluates:
- A knight on d4 (center) is stronger than a knight on a1 (corner)
- A rook on the 7th rank is more active than on the 1st rank
- A king on e1 is safer during the opening/middlegame
- A pawn on d5 is more advanced (and valuable) than on d2

---

## Table Organization

### Two-Phase Evaluation

Modern engines use separate tables for **midgame** and **endgame** phases:

```
Midgame (many pieces on board):
- Prioritize king safety
- Favor piece centralization
- Penalize exposed kings

Endgame (few pieces left):
- Activate the king (becomes strong piece)
- Favor advanced pawns
- Emphasize piece placement for checkmate
```

### Phase Transition

```cpp
// Simple phase calculation
int count_pieces(const Position& pos) {
    int total = 0;
    for (int c = 0; c < 2; ++c) {
        for (int p = 0; p < 6; ++p) {
            total += __builtin_popcountll(pos.pieces[c][p]);
        }
    }
    return total;
}

// Phase ranges from 0 (endgame) to 256 (midgame)
int get_phase(const Position& pos) {
    // Example: phase based on material count
    int material = count_pieces(pos);
    return std::min(256, material * 8);  // Scale to 0-256
}

// Interpolate between midgame and endgame
int interpolate_score(int mg_score, int eg_score, int phase) {
    return (mg_score * phase + eg_score * (256 - phase)) / 256;
}
```

---

## Pawn Tables

### Midgame Pawn Table
```cpp
// Bonuses for pawn placement (midgame)
// White's perspective: rank 0 = rank 1, rank 7 = rank 8
constexpr int PAWN_MG[64] = {
    0,   0,   0,   0,   0,   0,   0,   0,
    2,   4,   5,  10,  10,   5,   4,   2,   // Rank 2: modest advance bonus
    4,   8,  12,  16,  16,  12,   8,   4,   // Rank 3: better advanced
    6,  12,  16,  24,  24,  16,  12,   6,   // Rank 4: center pawns stronger
    8,  16,  24,  32,  32,  24,  16,   8,   // Rank 5: passed pawn territory
   12,  24,  36,  48,  48,  36,  24,  12,   // Rank 6: critical advancement
    0,   0,   0,   0,   0,   0,   0,   0,   // Rank 7: promotion (handled specially)
    0,   0,   0,   0,   0,   0,   0,   0,   // Rank 8: shouldn't exist
};

// Bonuses for pawn placement (endgame)
constexpr int PAWN_EG[64] = {
    0,   0,   0,   0,   0,   0,   0,   0,
   10,  10,  10,  10,  10,  10,  10,  10,   // Rank 2: passed pawn bonus
   20,  20,  20,  20,  20,  20,  20,  20,   // Rank 3: higher bonus
   30,  30,  30,  30,  30,  30,  30,  30,   // Rank 4
   40,  40,  40,  40,  40,  40,  40,  40,   // Rank 5: strong advancement
   60,  60,  60,  60,  60,  60,  60,  60,   // Rank 6: very strong
  100, 100, 100, 100, 100, 100, 100, 100,   // Rank 7: nearly winning
    0,   0,   0,   0,   0,   0,   0,   0,   // Rank 8
};
```

**Rationale**:
- Pawns gain value as they advance (less likely to be captured)
- Center pawns (d, e) get slight bonus for control
- Endgame heavily rewards advanced pawns (potential promotion)
- Rank 7 pawns are nearly queens (huge value in endgame)

---

## Knight Tables

### Midgame Knight Table
```cpp
// Knights love the center during middlegame
constexpr int KNIGHT_MG[64] = {
   -10,  -8,  -6,  -4,  -4,  -6,  -8, -10,
    -8,   0,   2,   4,   4,   2,   0,  -8,
    -6,   2,   6,   8,   8,   6,   2,  -6,
    -4,   4,   8,  10,  10,   8,   4,  -4,  // Rank 4: center squares
    -4,   4,   8,  10,  10,   8,   4,  -4,  // Rank 4: symmetric
    -6,   2,   6,   8,   8,   6,   2,  -6,
    -8,   0,   2,   4,   4,   2,   0,  -8,
   -10,  -8,  -6,  -4,  -4,  -6,  -8, -10,
};

// Endgame: knights still like center, but less critical
constexpr int KNIGHT_EG[64] = {
    -6,  -4,  -2,   0,   0,  -2,  -4,  -6,
    -4,   0,   2,   4,   4,   2,   0,  -4,
    -2,   2,   4,   6,   6,   4,   2,  -2,
     0,   4,   6,   8,   8,   6,   4,   0,
     0,   4,   6,   8,   8,   6,   4,   0,
    -2,   2,   4,   6,   6,   4,   2,  -2,
    -4,   0,   2,   4,   4,   2,   0,  -4,
    -6,  -4,  -2,   0,   0,  -2,  -4,  -6,
};
```

**Rationale**:
- Knights on corners (a1, h1, a8, h8) are almost dead: -10 penalty
- Knights on d4, e4, d5, e5: +10 bonus (optimal squares)
- Knights value center more in middlegame (more mobility)

---

## Bishop Tables

### Midgame Bishop Table
```cpp
// Bishops control long diagonals; prefer open positions
constexpr int BISHOP_MG[64] = {
    -4,  -2,  -2,  -2,  -2,  -2,  -2,  -4,
    -2,   0,   2,   2,   2,   2,   0,  -2,
    -2,   2,   4,   4,   4,   4,   2,  -2,
    -2,   2,   4,   6,   6,   4,   2,  -2,  // Rank 4: center diagonals
    -2,   2,   4,   6,   6,   4,   2,  -2,  // Rank 4
    -2,   2,   4,   4,   4,   4,   2,  -2,
    -2,   0,   2,   2,   2,   2,   0,  -2,
    -4,  -2,  -2,  -2,  -2,  -2,  -2,  -4,
};

constexpr int BISHOP_EG[64] = {
    -2,  -1,  -1,  -1,  -1,  -1,  -1,  -2,
    -1,   0,   1,   1,   1,   1,   0,  -1,
    -1,   1,   2,   2,   2,   2,   1,  -1,
    -1,   1,   2,   4,   4,   2,   1,  -1,
    -1,   1,   2,   4,   4,   2,   1,  -1,
    -1,   1,   2,   2,   2,   2,   1,  -1,
    -1,   0,   1,   1,   1,   1,   0,  -1,
    -2,  -1,  -1,  -1,  -1,  -1,  -1,  -2,
};
```

**Rationale**:
- Bishops on center diagonals (b1-h7, a2-g8): +4 bonus
- Bishops on long diagonals very valuable
- Slightly lower values in endgame compared to knights

---

## Rook Tables

### Midgame Rook Table
```cpp
// Rooks want open files and 7th rank (behind enemy pawns)
constexpr int ROOK_MG[64] = {
     0,   1,   2,   3,   3,   2,   1,   0,
     1,   2,   3,   4,   4,   3,   2,   1,
     0,   0,   0,   0,   0,   0,   0,   0,  // Rank 3: neutral
     0,   0,   0,   0,   0,   0,   0,   0,  // Rank 4
     0,   0,   0,   0,   0,   0,   0,   0,  // Rank 5
     0,   0,   0,   0,   0,   0,   0,   0,  // Rank 6
     5,   5,   5,   5,   5,   5,   5,   5,  // Rank 7: 7th rank bonus!
     0,   1,   2,   3,   3,   2,   1,   0,
};

// Endgame: rooks more flexible, centralization matters
constexpr int ROOK_EG[64] = {
    -4,  -2,   0,   0,   0,   0,  -2,  -4,
    -2,   0,   2,   2,   2,   2,   0,  -2,
     0,   2,   4,   4,   4,   4,   2,   0,  // Rank 3: centralize
     0,   2,   4,   6,   6,   4,   2,   0,  // Rank 4
     0,   2,   4,   6,   6,   4,   2,   0,  // Rank 5
     0,   2,   4,   4,   4,   4,   2,   0,  // Rank 6
    -2,   0,   2,   2,   2,   2,   0,  -2,
    -4,  -2,   0,   0,   0,   0,  -2,  -4,
};
```

**Rationale**:
- Rooks on 7th rank: +5 bonus (dominating position)
- Rooks on open files: implicit bonus via mobility
- Midgame: prefer 7th rank | Endgame: prefer center

---

## Queen Tables

### Midgame Queen Table
```cpp
// Queens are flexible; generally centralize but not as aggressive
constexpr int QUEEN_MG[64] = {
    -4,  -2,   0,   0,   0,   0,  -2,  -4,
    -2,   0,   2,   2,   2,   2,   0,  -2,
     0,   2,   4,   4,   4,   4,   2,   0,
     0,   2,   4,   6,   6,   4,   2,   0,  // Rank 4: center bonus
     0,   2,   4,   6,   6,   4,   2,   0,  // Rank 5
     0,   2,   4,   4,   4,   4,   2,   0,
    -2,   0,   2,   2,   2,   2,   0,  -2,
    -4,  -2,   0,   0,   0,   0,  -2,  -4,
};

constexpr int QUEEN_EG[64] = {
    -2,  -1,   0,   0,   0,   0,  -1,  -2,
    -1,   0,   1,   1,   1,   1,   0,  -1,
     0,   1,   2,   2,   2,   2,   1,   0,
     0,   1,   2,   4,   4,   2,   1,   0,
     0,   1,   2,   4,   4,   2,   1,   0,
     0,   1,   2,   2,   2,   2,   1,   0,
    -1,   0,   1,   1,   1,   1,   0,  -1,
    -2,  -1,   0,   0,   0,   0,  -1,  -2,
};
```

**Rationale**:
- Similar to rooks but slightly less aggressive on 7th rank
- Center control valuable but not critical
- Queens flexible piece: placement less critical than safety

---

## King Tables

### Midgame King Table (Safety First!)
```cpp
// During middlegame, king safety is CRITICAL
// Incentivize staying in corner (castled position)
constexpr int KING_MG[64] = {
   -40, -30, -30, -30, -30, -30, -30, -40,  // Rank 8
   -30, -20, -10, -10, -10, -10, -20, -30,  // Rank 7
   -20, -10,   0,   0,   0,   0, -10, -20,  // Rank 6
   -10,   0,   5,   5,   5,   5,   0, -10,  // Rank 5
     0,   5,  10,  10,  10,  10,   5,   0,  // Rank 4
    -5,   0,   5,  10,  10,   5,   0,  -5,  // Rank 3
   -30, -20, -10, -10, -10, -10, -20, -30,  // Rank 2
   -50, -40, -30, -30, -30, -30, -40, -50,  // Rank 1
};

// Better representation: castled king bonuses
constexpr int KING_MG_CASTLED[64] = {
    // Kingside castled (king on g1): safe in corner
     0,   0,   0,   0,   0,   0,   0,   0,
     0,   0,   0,   0,   0,   0,   0,   0,
    10,  10,   5,   0,   0,   5,  10,  10,
     5,   5,   0,   0,   0,   0,   5,   5,
     0,   0,   0,   0,   0,   0,   0,   0,
   -10, -10,   0,   0,   0,   0, -10, -10,
   -20, -10,   0,   0,   0,   0, -10, -20,
   -50, -40, -30, -20, -20, -30, -40, -50,
};
```

### Endgame King Table (Activation!)
```cpp
// In endgame with few pieces, king becomes powerful
// Incentivize king moving toward center
constexpr int KING_EG[64] = {
    -6,  -4,  -2,   0,   0,  -2,  -4,  -6,
    -4,   0,   2,   4,   4,   2,   0,  -4,
    -2,   2,   4,   6,   6,   4,   2,  -2,
     0,   4,   6,   8,   8,   6,   4,   0,  // Rank 4: center bonus
     0,   4,   6,   8,   8,   6,   4,   0,  // Rank 5
    -2,   2,   4,   6,   6,   4,   2,  -2,
    -4,   0,   2,   4,   4,   2,   0,  -4,
    -6,  -4,  -2,   0,   0,  -2,  -4,  -6,
};
```

**Rationale**:
- Midgame: king in corner (castled) is safest: -50 to +10 range
- Endgame: king in center is strongest: -6 to +8 range
- Opposite incentives for safety vs. activity!

---

## Implementation in C++

### Piece-Square Table Class
```cpp
class PieceSquareTables {
private:
    // Store both midgame and endgame tables for each piece
    std::array<std::array<int, 64>, 6> mg_tables;
    std::array<std::array<int, 64>, 6> eg_tables;
    
    static constexpr int PAWN_MG[64] = { /* ... */ };
    static constexpr int PAWN_EG[64] = { /* ... */ };
    static constexpr int KNIGHT_MG[64] = { /* ... */ };
    // ... etc for all pieces
    
public:
    PieceSquareTables() {
        // Initialize tables (can be hardcoded or loaded from file)
        std::copy(PAWN_MG, PAWN_MG + 64, mg_tables[PAWN].begin());
        std::copy(PAWN_EG, PAWN_EG + 64, eg_tables[PAWN].begin());
        std::copy(KNIGHT_MG, KNIGHT_MG + 64, mg_tables[KNIGHT].begin());
        // ... and so on
    }
    
    // Get score for a piece at a square (flip for black)
    int get_mg_score(PieceType pt, Square sq, Color color) const {
        if (color == WHITE) {
            return mg_tables[pt][sq];
        } else {
            return mg_tables[pt][sq ^ 56];  // Flip vertically for black
        }
    }
    
    int get_eg_score(PieceType pt, Square sq, Color color) const {
        if (color == WHITE) {
            return eg_tables[pt][sq];
        } else {
            return eg_tables[pt][sq ^ 56];
        }
    }
};
```

### Integration into Evaluation
```cpp
Score evaluate_position(const Position& pos, const PieceSquareTables& pst) {
    Score score = 0;
    int phase = get_phase(pos);  // 0 = endgame, 256 = midgame
    
    // Material + PST for each piece
    for (int color = 0; color < 2; ++color) {
        Color c = (Color)color;
        int sign = (c == WHITE) ? 1 : -1;
        
        for (int piece_type = 0; piece_type < 6; ++piece_type) {
            Bitboard bb = pos.pieces[color][piece_type];
            
            while (bb) {
                Square sq = pop_lsb(bb);
                
                // Material value
                int material_value = get_piece_value(piece_type);
                
                // PST value (interpolated)
                int mg_pst = pst.get_mg_score(piece_type, sq, c);
                int eg_pst = pst.get_eg_score(piece_type, sq, c);
                int pst_value = (mg_pst * phase + eg_pst * (256 - phase)) / 256;
                
                score += sign * (material_value + pst_value);
            }
        }
    }
    
    return score;
}
```

---

## Advanced Positional Bonuses

Beyond piece-square tables, consider these additions:

### 1. Pawn Structure Bonuses
```cpp
int evaluate_pawn_structure(const Position& pos) {
    int score = 0;
    
    // Passed pawns: huge bonus
    // - Pawn is passed if no enemy pawns on same file or adjacent files
    
    // Doubled pawns: small penalty
    // - Two pawns of same color on same file
    
    // Isolated pawns: small penalty
    // - Pawn with no friendly pawns on adjacent files
    
    // Connected pawns: small bonus
    // - Pawns supporting each other
    
    return score;
}
```

### 2. Rook Activity
```cpp
int evaluate_rook_activity(const Position& pos) {
    int score = 0;
    
    // Open files: rooks on open files get bonus
    // Semi-open files: rooks opposing enemy pawns
    // Rooks on 7th rank: already in PST but can add context bonus
    
    return score;
}
```

### 3. King Safety
```cpp
int evaluate_king_safety(const Position& pos) {
    int score = 0;
    
    // Pawn shield: pawns in front of king (f, g, h for white)
    // - Bonus if intact, penalty if compromised
    
    // Open lines to king: bonus for attacking side
    // - Penalty for king if lines open
    
    return score;
}
```

### 4. Piece Mobility
```cpp
int count_piece_mobility(const Position& pos, Color color) {
    int mobility = 0;
    
    // Count number of squares each piece can move to
    // Knights: typically 4-8 squares
    // Bishops: 0-13 squares (depends on blockades)
    // Rooks: 0-14 squares
    // Queens: 0-27 squares
    
    // Assign small bonus per available square (5-10 cp per square)
    
    return mobility;
}
```

---

## Parameter Tuning

### Manual Adjustment
```cpp
// If engine places knights on edges: increase corner penalties
// If engine hangs material in tactical positions: increase piece mobility bonus
// If engine castles too late: increase king PST penalizes exposed kings
```

### Automated Tuning (Advanced)
Use tools like TEXEL's Tuning Method or Ordinal Optimizer to automatically adjust table values based on game results.

---

## Example Complete Tables

Here's a simplified but functional set of tables:

```cpp
// Simplified piece values
constexpr int PIECE_VALUES[6] = {
    100,   // Pawn
    325,   // Knight
    335,   // Bishop
    500,   // Rook
    900,   // Queen
    0      // King (infinite)
};

// Simple centralization bonus (applies to most pieces)
constexpr int CENTER_BONUS[64] = {
    0, 0, 0, 0, 0, 0, 0, 0,
    0, 1, 1, 1, 1, 1, 1, 0,
    0, 1, 2, 2, 2, 2, 1, 0,
    0, 1, 2, 3, 3, 2, 1, 0,
    0, 1, 2, 3, 3, 2, 1, 0,
    0, 1, 2, 2, 2, 2, 1, 0,
    0, 1, 1, 1, 1, 1, 1, 0,
    0, 0, 0, 0, 0, 0, 0, 0,
};
```

---

## Summary Table

| Piece | Midgame Focus | Endgame Focus |
|-------|---------------|---------------|
| **Pawn** | Modest advance bonus | Heavy advance bonus (near promotion) |
| **Knight** | Centralization | Centralization (less critical) |
| **Bishop** | Center diagonals | Center control |
| **Rook** | 7th rank | Centralization + files |
| **Queen** | Center (moderate) | Center + activity |
| **King** | Safety in corner | Activate toward center |

