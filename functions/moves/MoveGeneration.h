#ifndef MOVE_GENERATION_H
#define MOVE_GENERATION_H

#include <stdint.h>
#include <stdbool.h>

#include "core/types.h"
#include "data/Position.h"

// max theoretical moves per position
#define MAX_MOVES 218

#ifdef __cplusplus
extern "C" {
#endif

// Main entry point for legal move generation
int generate_legal_moves(const Position* pos, Move* out_moves);

// Generate capture moves only
int generate_capture_moves(const Position* pos, Move* out_moves);

// Get all squares that can capture a specific target square
Bitboard get_capture_mask(const Position* pos, Square target_square, Color attacking_color);

#ifdef __cplusplus
}
#endif

#endif // MOVE_GENERATION_H
