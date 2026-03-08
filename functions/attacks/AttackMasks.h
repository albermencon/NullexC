#ifndef ATTACK_MASKS_H
#define ATTACK_MASKS_H

#include <stdint.h>
#include "core/types.h"
#include "core/bitboard.h"
#ifdef USE_BMI2
#include <immintrin.h>
#endif

// Attack tables
extern uint64_t KNIGHT_ATTACKS[64];
extern uint64_t KING_ATTACKS[64];
extern uint64_t PAWN_ATTACKS[2][64]; // [color][square]

// Sliding pieces attack tables
extern uint64_t ROOK_MAGICS[64];
extern uint64_t BISHOP_MAGICS[64];
extern int ROOK_BITS[64];
extern int BISHOP_BITS[64];
extern uint64_t* ROOK_ATTACKS[64];
extern uint64_t* BISHOP_ATTACKS[64];
extern uint64_t ROOK_MASKS[64];
extern uint64_t BISHOP_MASKS[64];

// Initialization functions
void initialize_attack_masks(void);
void free_attack_tables(void);

// Accessors (static inline)
static inline Bitboard knight_attacks(Square square) { return KNIGHT_ATTACKS[square]; }
static inline Bitboard king_attacks(Square square) { return KING_ATTACKS[square]; }
static inline Bitboard pawn_attacks(Square square, Color color) { return PAWN_ATTACKS[color][square]; }
static inline Bitboard rook_attacks(Square sq, Bitboard occupied) {
#ifdef USE_BMI2
    return ROOK_ATTACKS[sq][_pext_u64(occupied, ROOK_MASKS[sq])];
#else
    occupied &= ROOK_MASKS[sq];
    occupied *= ROOK_MAGICS[sq];
    occupied >>= (64 - ROOK_BITS[sq]);
    return ROOK_ATTACKS[sq][(int)occupied];
#endif
}
static inline Bitboard bishop_attacks(Square sq, Bitboard occupied) {
#ifdef USE_BMI2
    return BISHOP_ATTACKS[sq][_pext_u64(occupied, BISHOP_MASKS[sq])];
#else
    occupied &= BISHOP_MASKS[sq];
    occupied *= BISHOP_MAGICS[sq];
    occupied >>= (64 - BISHOP_BITS[sq]);
    return BISHOP_ATTACKS[sq][(int)occupied];
#endif
}
static inline Bitboard queen_attacks(Square sq, Bitboard occupied) {
    return rook_attacks(sq, occupied) | bishop_attacks(sq, occupied);
}

#endif
