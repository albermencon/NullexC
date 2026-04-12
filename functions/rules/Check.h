#ifndef CHECK_H
#define CHECK_H

#include <stdbool.h>
#include <stdint.h>

#include "core/types.h"
#include "data/Position.h"
#include "functions/attacks/AttackMasks.h"

/**
 * Checks whether the given square is attacked by the given color.
 */
bool isSquareAttacked(const Position* pos, Square sq, Color byColor);

// all checkers hitting `king` of color `us` from `them`
static inline Bitboard get_checkers(const Position* pos, Color us, Square king, Color them) {
    return
        (pawn_attacks(king, us) &
            pos->bitboards[piece_type_to_bitboard_index(them, PIECE_PAWN)]) |
        (knight_attacks(king) &
            pos->bitboards[piece_type_to_bitboard_index(them, PIECE_KNIGHT)]) |
        (bishop_attacks(king, pos->occupied) &
            (pos->bitboards[piece_type_to_bitboard_index(them, PIECE_BISHOP)] |
                pos->bitboards[piece_type_to_bitboard_index(them, PIECE_QUEEN)])) |
        (rook_attacks(king, pos->occupied) &
            (pos->bitboards[piece_type_to_bitboard_index(them, PIECE_ROOK)] |
                pos->bitboards[piece_type_to_bitboard_index(them, PIECE_QUEEN)]));
}

// pinned pieces of color `us` relative to their `king`
static inline Bitboard compute_pinned(const Position* pos, Color us, Square king) {
    Color    them = opposite(us);

    // No sliders = no pins possible — skip all work
    //Bitboard enemySliders =
    //    pos->bitboards[piece_type_to_bitboard_index(them, PIECE_BISHOP)] |
    //    pos->bitboards[piece_type_to_bitboard_index(them, PIECE_ROOK)] |
    //    pos->bitboards[piece_type_to_bitboard_index(them, PIECE_QUEEN)];
    //if (!enemySliders) return 0ULL;

    Bitboard ourPieces = (us == WHITE) ? pos->whitePieces : pos->blackPieces;
    Bitboard pinned = 0ULL;

    // X-ray through our own pieces to find diagonal pinners
    Bitboard occXray = pos->occupied & ~ourPieces;

    Bitboard diagPinners =
        bishop_attacks(king, occXray) &
        (pos->bitboards[piece_type_to_bitboard_index(them, PIECE_BISHOP)] |
            pos->bitboards[piece_type_to_bitboard_index(them, PIECE_QUEEN)]);

    Bitboard tmp = diagPinners;
    while (tmp) {
        Square   pinner = (Square)pop_lsb(&tmp);
        Bitboard between = BetweenBB[king][pinner] & pos->occupied;
        if (between && !(between & (between - 1)) && (between & ourPieces))
            pinned |= between;
    }

    // X-ray through our own pieces to find straight pinners
    Bitboard straightPinners =
        rook_attacks(king, occXray) &
        (pos->bitboards[piece_type_to_bitboard_index(them, PIECE_ROOK)] |
            pos->bitboards[piece_type_to_bitboard_index(them, PIECE_QUEEN)]);

    tmp = straightPinners;
    while (tmp) {
        Square   pinner = (Square)pop_lsb(&tmp);
        Bitboard between = BetweenBB[king][pinner] & pos->occupied;
        if (between && !(between & (between - 1)) && (between & ourPieces))
            pinned |= between;
    }

    return pinned;
}

#endif // CHECK_H
