#ifndef EVALUATE_H
#define EVALUATE_H

#include <immintrin.h>
#include <stdalign.h>
#include "data/Position.h"

static const int16_t PIECE_WEIGHTS[NUM_BITBOARDS] = {
    100, 300, 320, 500, 900, 0,      // White
    -100, -300, -320, -500, -900, 0   // Black
};

// White perspective 
// From Sebastian Lague (https://github.com/SebLague/Chess-Coding-Adventure)
static const int16_t PST_MG[6][64] = {
    // PAWN
    {  0,  0,  0,  0,  0,  0,  0,  0,
      50, 50, 50, 50, 50, 50, 50, 50,
      10, 10, 20, 30, 30, 20, 10, 10,
       5,  5, 10, 25, 25, 10,  5,  5,
       0,  0,  0, 20, 20,  0,  0,  0,
       5, -5,-10,  0,  0,-10, -5,  5,
       5, 10, 10,-20,-20, 10, 10,  5,
       0,  0,  0,  0,  0,  0,  0,  0 },
    // KNIGHT
    { -50,-40,-30,-30,-30,-30,-40,-50,
      -40,-20,  0,  0,  0,  0,-20,-40,
      -30,  0, 10, 15, 15, 10,  0,-30,
      -30,  5, 15, 20, 20, 15,  5,-30,
      -30,  0, 15, 20, 20, 15,  0,-30,
      -30,  5, 10, 15, 15, 10,  5,-30,
      -40,-20,  0,  5,  5,  0,-20,-40,
      -50,-40,-30,-30,-30,-30,-40,-50 },
    // BISHOP
    { -20,-10,-10,-10,-10,-10,-10,-20,
      -10,  0,  0,  0,  0,  0,  0,-10,
      -10,  0,  5, 10, 10,  5,  0,-10,
      -10,  5,  5, 10, 10,  5,  5,-10,
      -10,  0, 10, 10, 10, 10,  0,-10,
      -10, 10, 10, 10, 10, 10, 10,-10,
      -10,  5,  0,  0,  0,  0,  5,-10,
      -20,-10,-10,-10,-10,-10,-10,-20 },
    // ROOK
    {  0,  0,  0,  0,  0,  0,  0,  0,
       5, 10, 10, 10, 10, 10, 10,  5,
      -5,  0,  0,  0,  0,  0,  0, -5,
      -5,  0,  0,  0,  0,  0,  0, -5,
      -5,  0,  0,  0,  0,  0,  0, -5,
      -5,  0,  0,  0,  0,  0,  0, -5,
      -5,  0,  0,  0,  0,  0,  0, -5,
       0,  0,  0,  5,  5,  0,  0,  0 },
    // QUEEN
    { -20,-10,-10, -5, -5,-10,-10,-20,
      -10,  0,  0,  0,  0,  0,  0,-10,
      -10,  0,  5,  5,  5,  5,  0,-10,
      -5,   0,  5,  5,  5,  5,  0, -5,
       0,   0,  5,  5,  5,  5,  0, -5,
      -10,  5,  5,  5,  5,  5,  0,-10,
      -10,  0,  5,  0,  0,  0,  0,-10,
      -20,-10,-10, -5, -5,-10,-10,-20 },
    // KING
    { -80,-70,-70,-70,-70,-70,-70,-80,
      -60,-60,-60,-60,-60,-60,-60,-60,
      -40,-50,-50,-60,-60,-50,-50,-40,
      -30,-40,-40,-50,-50,-40,-40,-30,
      -20,-30,-30,-40,-40,-30,-30,-20,
      -10,-20,-20,-20,-20,-20,-20,-10,
       20, 20, -5, -5, -5, -5, 20, 20,
       20, 30, 10,  0,  0, 10, 30, 20 }
};

static inline int get_pst_score(int piece, Square sq) {
    if (piece == -1) return 0;
    int type = PIECE_INDEX_TO_TYPE[piece];
    // White pieces (0-5) flip the rank (sq ^ 56). Black reads it as is.
    int table_sq = (piece <= 5) ? (sq ^ 56) : sq;
    return PST_MG[type][table_sq];
}

static inline int piece_value(int piece, Square sq) {
    int val = PIECE_WEIGHTS[piece];
    if (piece <= 5) return val + get_pst_score(piece, sq);
    else            return val - get_pst_score(piece, sq);
}

static inline int non_pawn_weight(int piece) {
    if (piece == -1 || is_pawn(piece) || is_king(piece)) return 0;
    int val = PIECE_WEIGHTS[piece];
    return val > 0 ? val : -val;
}

int evaluate(const Position* pos);

#endif // EVALUATE_H
