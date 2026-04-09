#include "functions/rules/Check.h"
#include "functions/attacks/AttackMasks.h"

#include <string.h> // for memset

bool isSquareAttacked(const Position* pos, Square sq, Color byColor) {
    uint64_t occ = pos->occupied;

    if (pawn_attacks(sq, opposite(byColor)) &
        pos->bitboards[piece_type_to_bitboard_index(byColor, PIECE_PAWN)])
        return true;

    if (knight_attacks(sq) &
        pos->bitboards[piece_type_to_bitboard_index(byColor, PIECE_KNIGHT)])
        return true;

    uint64_t bishopAtt = bishop_attacks(sq, occ);
    if (bishopAtt & (pos->bitboards[piece_type_to_bitboard_index(byColor, PIECE_BISHOP)] |
                     pos->bitboards[piece_type_to_bitboard_index(byColor, PIECE_QUEEN)]))
        return true;

    uint64_t rookAtt = rook_attacks(sq, occ);
    if (rookAtt & (pos->bitboards[piece_type_to_bitboard_index(byColor, PIECE_ROOK)] |
                   pos->bitboards[piece_type_to_bitboard_index(byColor, PIECE_QUEEN)]))
        return true;

    if (king_attacks(sq) &
        pos->bitboards[piece_type_to_bitboard_index(byColor, PIECE_KING)])
        return true;

    return false;
}
