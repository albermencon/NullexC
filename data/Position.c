// Position.c
#include "data/Position.h"
#include "functions/rules/Check.h"
#include "core/bitboard.h"
#include "functions/evaluation/Evaluate.h"
#include <string.h>  // for memcpy

Bitboard LineBB[SQUARE_NB][SQUARE_NB];
Bitboard BetweenBB[SQUARE_NB][SQUARE_NB];

void Position_init(Position* pos,
                   const uint64_t bitboards[NUM_BITBOARDS],
                   bool whiteToMove,
                   uint8_t castlingRights,
                   short enPassantSquare,
                   short halfMoveClock,
                   int fullMoveCounter,
                   uint64_t zobristHash) {
    // Copy piece bitboards
    memcpy(pos->bitboards, bitboards, sizeof(uint64_t) * NUM_BITBOARDS);

    pos->whiteToMove = whiteToMove;
    pos->castlingRights = castlingRights;
    pos->enPassantSquare = enPassantSquare;
    pos->halfmoveClock = halfMoveClock;
    pos->fullmoveNumber = fullMoveCounter;
    pos->zobristHash = zobristHash;

    // Compute side-specific and occupied bitboards
    uint64_t white = 0ULL;
    for (int i = 0; i <= 5; i++) {
        white |= pos->bitboards[i];
    }

    uint64_t black = 0ULL;
    for (int i = 6; i < NUM_BITBOARDS; i++) {
        black |= pos->bitboards[i];
    }

    pos->whitePieces = white;
    pos->blackPieces = black;
    pos->occupied = white | black;

    // Fill squarePiece array
    for (int sq = 0; sq < 64; sq++) {
        pos->squarePiece[sq] = -1; // empty
        for (int i = 0; i < NUM_BITBOARDS; i++) {
            if (pos->bitboards[i] & (1ULL << sq)) {
                pos->squarePiece[sq] = i;
                break;
            }
        }
    }

    pos->kingSquare[WHITE] = (Square)lsb(pos->bitboards[W_K]);
    pos->kingSquare[BLACK] = (Square)lsb(pos->bitboards[B_K]);
}

bool aligned(Square s1, Square s2, PieceType pt) {
    int r1 = s1 >> 3, f1 = s1 & 7;
    int r2 = s2 >> 3, f2 = s2 & 7;

    if (pt == PIECE_ROOK)
        return r1 == r2 || f1 == f2;
    else
        return (r1 - f1) == (r2 - f2) || (r1 + f1) == (r2 + f2);
}

Bitboard between_squares(Square s1, Square s2) {
    Bitboard b = 0;

    int r1 = s1 >> 3, f1 = s1 & 7;
    int r2 = s2 >> 3, f2 = s2 & 7;

    int dr = (r2 > r1) ? 1 : (r2 < r1 ? -1 : 0);
    int df = (f2 > f1) ? 1 : (f2 < f1 ? -1 : 0);

    int r = r1 + dr;
    int f = f1 + df;

    while (r != r2 || f != f2) {
        b |= (1ULL << (r * 8 + f));
        r += dr;
        f += df;
    }

    b &= ~((1ULL << s1) | (1ULL << s2));

    return b;
}

void init_line_between_bb() {
    for (Square s1 = 0; s1 < SQUARE_NB; ++s1) {
        for (Square s2 = 0; s2 < SQUARE_NB; ++s2) {
            LineBB[s1][s2] = 0;
            BetweenBB[s1][s2] = 0;
            if (s1 == s2) continue;

            if (aligned(s1, s2, PIECE_ROOK)) {
                LineBB[s1][s2] =
                    (rook_attacks(s1, 0ULL) & rook_attacks(s2, 0ULL))
                    | (1ULL << s1) | (1ULL << s2);
                BetweenBB[s1][s2] = between_squares(s1, s2); // safe: same rank/file
            }
            else if (aligned(s1, s2, PIECE_BISHOP)) {
                LineBB[s1][s2] =
                    (bishop_attacks(s1, 0ULL) & bishop_attacks(s2, 0ULL))
                    | (1ULL << s1) | (1ULL << s2);
                BetweenBB[s1][s2] = between_squares(s1, s2); // safe: same diagonal
            }
            // else: not aligned LineBB=0, BetweenBB=0
        }
    }
}

void position_setup_root(Position* pos, StateInfo* root_state) {
    Color  us = pos->whiteToMove ? WHITE : BLACK;
    Color  them = opposite(us);
    Square king = pos->kingSquare[us];

    root_state->prev = NULL;
    root_state->checkersBB = get_checkers(pos, us, king, them);
    root_state->pinners[us] = compute_pinned(pos, us, pos->kingSquare[us]);
    root_state->pinners[them] = compute_pinned(pos, them, pos->kingSquare[them]);
    root_state->prevCastling = pos->castlingRights;
    root_state->prevEPSq = pos->enPassantSquare;
    root_state->prevHalfMove = pos->halfmoveClock;
    root_state->prevZobrist = pos->zobristHash;

    pos->st = root_state;

    // Initialize the PST and value
    for (int i = 0; i < NUM_BITBOARDS; i++)
    {
        Bitboard bb = pos->bitboards[i];
        while (bb)
        {
            Square sq = pop_lsb(&bb);
            root_state->eval += piece_value(i, sq);
            if (i != W_K && i != B_K)
            {
                if (i <= 5) root_state->non_pawn_white++;
                else root_state->non_pawn_black++;
            }
        }
    }
}
