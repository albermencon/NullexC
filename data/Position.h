#ifndef POSITION_H
#define POSITION_H

#include <stdint.h>
#include <stdbool.h>
#include "core/types.h"
#include "functions/attacks/AttackMasks.h"

#define NUM_BITBOARDS 12

extern Bitboard LineBB[SQUARE_NB][SQUARE_NB];
extern Bitboard BetweenBB[SQUARE_NB][SQUARE_NB];

typedef struct StateInfo StateInfo;

struct StateInfo {
    Move    move;
    int8_t  capturedPiece;
    int8_t movingPiece;
    uint8_t prevCastling;
    short   prevEPSq;
    short   prevHalfMove;
    uint64_t prevZobrist;
    bool    isNullMove;
    int non_pawn_white;
    int non_pawn_black;
    int eval;
    
    Bitboard checkersBB; // enemy pieces currently giving check
    Bitboard pinners[COLOR_NB]; // pinned pieces per side
    Bitboard checkSquares[PIECE_NB];
    Bitboard blockersForKing[COLOR_NB];

    StateInfo* prev;
};

typedef struct {
    Bitboard bitboards[NUM_BITBOARDS];
    Bitboard occupied;
    Bitboard whitePieces;
    Bitboard blackPieces;
    int8_t squarePiece[SQUARE_NB];
    Square   kingSquare[COLOR_NB];
    bool whiteToMove;
    uint8_t castlingRights;
    short enPassantSquare;
    short halfmoveClock;
    uint64_t zobristHash;
    int fullmoveNumber;
    StateInfo* st;
} Position;

void Position_init(Position* pos,
    const uint64_t bitboards[NUM_BITBOARDS],
    bool whiteToMove,
    uint8_t castlingRights,
    short enPassantSquare,
    short halfMoveClock,
    int fullMoveCounter,
    uint64_t zobristHash);

void init_line_between_bb();

Bitboard between_squares(Square s1, Square s2);
bool aligned(Square s1, Square s2, PieceType pt);

static inline Bitboard Position_friendlyPieces(const Position* pos) {
    return pos->whiteToMove ? pos->whitePieces : pos->blackPieces;
}

static inline Bitboard Position_enemyPieces(const Position* pos) {
    return pos->whiteToMove ? pos->blackPieces : pos->whitePieces;
}

static inline int Position_PieceAt(const Position* pos, int square) {
    return pos->squarePiece[square];
}

static inline void add_piece_bb(Position* pos, int pieceIndex, int sq) {
    Bitboard b = (1ULL << sq);
    pos->bitboards[pieceIndex] |= b;

    if (pieceIndex <= 5)
        pos->whitePieces |= b;
    else
        pos->blackPieces |= b;

    pos->occupied |= b;
    pos->squarePiece[sq] = pieceIndex;
    if (pieceIndex == W_K) pos->kingSquare[WHITE] = sq;
    if (pieceIndex == B_K) pos->kingSquare[BLACK] = sq;
}

static inline void remove_piece_bb(Position* pos, int pieceIndex, int sq) {
    Bitboard b = (1ULL << sq);

    pos->bitboards[pieceIndex] &= ~b;

    if (pieceIndex <= 5)
        pos->whitePieces &= ~b;
    else
        pos->blackPieces &= ~b;

    pos->occupied &= ~b;
    pos->squarePiece[sq] = -1;
    if (pieceIndex == W_K) pos->kingSquare[WHITE] = SQUARE_NB; // invalid sentinel
    if (pieceIndex == B_K) pos->kingSquare[BLACK] = SQUARE_NB;
}

static inline Bitboard between_bb(Square s1, Square s2) {
    return BetweenBB[s1][s2];
}

void position_setup_root(Position* pos, StateInfo* root_state);

#endif
