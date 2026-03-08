#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

typedef uint64_t Key;
typedef uint64_t Bitboard;
typedef uint64_t ZobristKey;

#define MAX_MOVES 218
#define MAX_PLY   246

typedef enum {
    WHITE = 0,
    BLACK = 1,
    COLOR_NB = 2
} Color;

// Lookup table for piece index (0–11) to color
// 0–5 are white pieces, 6–11 are black pieces
static const Color COLOR_BY_PIECE[12] = {
    WHITE, WHITE, WHITE,
    WHITE, WHITE, WHITE,
    BLACK, BLACK, BLACK,
    BLACK, BLACK, BLACK
};

//typedef enum {} CastlingRights;

typedef uint16_t Value; // Max 65536

typedef enum {
    PIECE_PAWN = 0,
    PIECE_KNIGHT = 1,
    PIECE_BISHOP = 2,
    PIECE_ROOK = 3,
    PIECE_QUEEN = 4,
    PIECE_KING = 5,
    PIECE_NB,
    PIECE_NONE = -1
} PieceType;

static const PieceType PIECE_INDEX_TO_TYPE[12] = {
    PIECE_PAWN, PIECE_KNIGHT, PIECE_BISHOP, PIECE_ROOK, PIECE_QUEEN, PIECE_KING,
    PIECE_PAWN, PIECE_KNIGHT, PIECE_BISHOP, PIECE_ROOK, PIECE_QUEEN, PIECE_KING
};

typedef enum {
    W_P = 0, W_N, W_B, W_R, W_Q, W_K,
    B_P, B_N, B_B, B_R, B_Q, B_K
} PieceIndex;

typedef uint8_t Square;

#define SQUARE_NB 64
enum Squares {
    SQ_A1 = 0, SQ_B1, SQ_C1, SQ_D1, SQ_E1, SQ_F1, SQ_G1, SQ_H1,
    SQ_A2, SQ_B2, SQ_C2, SQ_D2, SQ_E2, SQ_F2, SQ_G2, SQ_H2,
    SQ_A3, SQ_B3, SQ_C3, SQ_D3, SQ_E3, SQ_F3, SQ_G3, SQ_H3,
    SQ_A4, SQ_B4, SQ_C4, SQ_D4, SQ_E4, SQ_F4, SQ_G4, SQ_H4,
    SQ_A5, SQ_B5, SQ_C5, SQ_D5, SQ_E5, SQ_F5, SQ_G5, SQ_H5,
    SQ_A6, SQ_B6, SQ_C6, SQ_D6, SQ_E6, SQ_F6, SQ_G6, SQ_H6,
    SQ_A7, SQ_B7, SQ_C7, SQ_D7, SQ_E7, SQ_F7, SQ_G7, SQ_H7,
    SQ_A8, SQ_B8, SQ_C8, SQ_D8, SQ_E8, SQ_F8, SQ_G8, SQ_H8
};

typedef enum {
    FILE_A,
    FILE_B,
    FILE_C,
    FILE_D,
    FILE_E,
    FILE_F,
    FILE_G,
    FILE_H,
    FILE_NB
} File;

typedef enum {
    RANK_1,
    RANK_2,
    RANK_3,
    RANK_4,
    RANK_5,
    RANK_6,
    RANK_7,
    RANK_8,
    RANK_NB
} Rank;

typedef enum {
    NORTH = 8,
    EAST  = 1,
    SOUTH = -NORTH,
    WEST  = -EAST,

    NORTH_EAST = NORTH + EAST,
    SOUTH_EAST = SOUTH + EAST,
    SOUTH_WEST = SOUTH + WEST,
    NORTH_WEST = NORTH + WEST
} PAWN_DIRECTION;

// Bit Layout:
// Bits  0–5   : destination square (0–63)
// Bits  6–11  : origin square      (0–63)
// Bits 12–13  : promotion piece type (00=knight, 01=bishop, 10=rook, 11=queen)
// Bits 14–15  : special flag: 00=normal, 01=promotion, 10=en passant, 11=castling
typedef uint16_t Move;

// Constants for encoding
#define NORMAL     0b00
#define PROMOTION  0b01
#define EN_PASSANT 0b10
#define CASTLING   0b11

#define PROMO_KNIGHT 0b00
#define PROMO_BISHOP 0b01
#define PROMO_ROOK   0b10
#define PROMO_QUEEN  0b11

#define DEST_SHIFT   0
#define ORIGIN_SHIFT 6
#define PROMO_SHIFT  12
#define SPECIAL_SHIFT 14

#define SQUARE_MASK  0x3F
#define PROMO_MASK   0x03
#define SPECIAL_MASK 0x03

// Move Macros
#define MOVE_NONE ((Move)0xFFFF)
#define MOVE_ENCODE(from,to,promo,flag) \
    ( (Move)( \
        ( ((uint16_t)((to)   & SQUARE_MASK)) << DEST_SHIFT )    | \
        ( ((uint16_t)((from) & SQUARE_MASK)) << ORIGIN_SHIFT )  | \
        ( ((uint16_t)((promo) & PROMO_MASK))   << PROMO_SHIFT )  | \
        ( ((uint16_t)((flag)  & SPECIAL_MASK)) << SPECIAL_SHIFT ) ) )

#define MOVE_NORMAL(from,to)        MOVE_ENCODE(from,to,0, NORMAL)
#define MOVE_PROMOTION(from,to,p)   MOVE_ENCODE(from,to,p, PROMOTION)
#define MOVE_EP(from,to)            MOVE_ENCODE(from,to,0, EN_PASSANT)
#define MOVE_CASTLE(from,to)        MOVE_ENCODE(from,to,0, CASTLING)

#define MOVE_FROM(m)   ( ( (m) >> ORIGIN_SHIFT ) & SQUARE_MASK )
#define MOVE_TO(m)     ( ( (m) >> DEST_SHIFT   ) & SQUARE_MASK )
#define MOVE_PROMO(m)  ( ( (m) >> PROMO_SHIFT  ) & PROMO_MASK )
#define MOVE_FLAG(m)   ( ( (m) >> SPECIAL_SHIFT) & SPECIAL_MASK )

static inline Move move_null(void) { return MOVE_NONE; }

static inline Move move_make(int from, int to, int promo, int flag) { return MOVE_ENCODE(from,to,promo,flag); }

static inline Square move_from(Move m) { return (Square)MOVE_FROM(m); }

static inline Square move_to(Move m)   { return (Square)MOVE_TO(m); }

static inline int move_promo_type(Move m) { return (int)MOVE_PROMO(m); }

static inline int move_special_flag(Move m) { return (int)MOVE_FLAG(m); }

static inline Color opposite(Color c) { return (Color)(c ^ BLACK); }

static inline Color piece_index_color(int pieceIndex) { return COLOR_BY_PIECE[pieceIndex]; }

static inline int piece_type_to_bitboard_index(Color color, PieceType type) { return color * 6 + type; } // Computes bitboard index (0–11) given color and piece type

static inline PieceType piece_type_from_index(int index) { return PIECE_INDEX_TO_TYPE[index]; } // Returns the piece type from a combined piece index (0–11)

static inline bool is_pawn(int pieceIndex) { return pieceIndex == 0 || pieceIndex == 6; } // Checks if a piece index represents a pawn (white or black)

static inline bool is_king(int pieceIndex) { return pieceIndex == 5 || pieceIndex == 11; } // Checks if a piece index represents a king (white or black)

static inline bool is_rook(int pieceIndex) { return pieceIndex == 3 || pieceIndex == 9; } // Checks if a piece index represents a rook (white or black)

static inline void square_validate(int sq) { assert(sq >= SQ_A1 && sq < SQUARE_NB); }

static inline Square square_of(int sq) { square_validate(sq); return (Square)sq; }

static inline Square square_of_rank_file(Rank rank, File file) { return (Square)((rank << 3) + file); }

static inline Rank square_rank(Square sq) { return (Rank)(sq >> 3); } // it's the same as: / 8

static inline File square_file(Square sq) { return (File)(sq & 7); }  // it's the same as: % 8

static inline Rank relative_rank(Color color, Rank rank) { return (Rank)(rank ^ (color * 7)); }

static inline Rank relative_rank_from_square(Color color, Square sq) { return (Rank)(relative_rank(color, square_rank(sq)));}

static inline Bitboard square_to_bitboard(Square sq) { return 1ULL << sq; }

static inline PAWN_DIRECTION pawn_direction(Color color) { return color == WHITE ? NORTH : SOUTH; }

static inline PieceType get_promotion_piece_type(Move move) {
    switch (move_promo_type(move)) {
        case PROMO_KNIGHT: return PIECE_KNIGHT;
        case PROMO_BISHOP: return PIECE_BISHOP;
        case PROMO_ROOK:   return PIECE_ROOK;
        case PROMO_QUEEN:  return PIECE_QUEEN;
        default:           return PIECE_NONE;
    }
}

#endif // TYPES_H
