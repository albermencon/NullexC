#include "SSE.h"
#include "core/types.h"
#include "functions/attacks/AttackMasks.h"
#include <assert.h>

// Calculate all pieces attacking a specific square
static inline Bitboard get_attackers(const Position* pos, Square sq, Bitboard occupied) {
    return (pawn_attacks(sq, BLACK) & pos->bitboards[W_P]) |
           (pawn_attacks(sq, WHITE) & pos->bitboards[B_P]) |
           (knight_attacks(sq) & (pos->bitboards[W_N] | pos->bitboards[B_N])) |
           (bishop_attacks(sq, occupied) & (pos->bitboards[W_B] | pos->bitboards[B_B] | pos->bitboards[W_Q] | pos->bitboards[B_Q])) |
           (rook_attacks(sq, occupied) & (pos->bitboards[W_R] | pos->bitboards[B_R] | pos->bitboards[W_Q] | pos->bitboards[B_Q])) |
           (king_attacks(sq) & (pos->bitboards[W_K] | pos->bitboards[B_K]));
}

static const int SEE_VALUES[PIECE_NB] = {
    100, 300, 300, 500, 900, 20000 // P, N, B, R, Q, K
};

int SEE(const Position* pos, Move move, int threshold) 
{
    Square from = move_from(move);
    Square to = move_to(move);
    int move_flag = move_special_flag(move);
    
    int from_pc = Position_PieceAt(pos, from);
    int to_pc = Position_PieceAt(pos, to);
    assert(from_pc != -1);
    assert(to >= 0 && to < 64);

    int gain[32];
    int d = 0;

    gain[0] = (move_flag == EN_PASSANT) ? SEE_VALUES[PIECE_PAWN] : 
              (to_pc != PIECE_NONE) ? SEE_VALUES[PIECE_INDEX_TO_TYPE[to_pc]] : 0;

    // Fast-path prune: Evaluate best-case scenario immediately
    if (gain[0] - SEE_VALUES[PIECE_INDEX_TO_TYPE[from_pc]] >= threshold) {
        return gain[0]; 
    }

    Bitboard occupied = pos->occupied;
    Bitboard attackers = get_attackers(pos, to, occupied);

    // Make initial move locally
    occupied ^= (1ULL << from);
    if (move_flag == EN_PASSANT) {
        Square ep_cap_sq = to + (piece_index_color(from_pc) == WHITE ? SOUTH : NORTH);
        occupied ^= (1ULL << ep_cap_sq);
    }
    
    int next_victim = (move_flag == PROMOTION) ? get_promotion_piece_type(move) : PIECE_INDEX_TO_TYPE[from_pc];
    Color side_to_move = opposite(piece_index_color(from_pc));

    while (1) 
    {
        d++;
        Bitboard side_attackers = attackers & (side_to_move == WHITE ? pos->whitePieces : pos->blackPieces) & occupied;
        
        if (!side_attackers) break;

        int attacker_type = PIECE_KING;
        Bitboard bb = 0;

        // Unrolled evaluation for least valuable attacker
        if      ((bb = side_attackers & pos->bitboards[piece_type_to_bitboard_index(side_to_move, PIECE_PAWN)]))   { attacker_type = PIECE_PAWN; }
        else if ((bb = side_attackers & pos->bitboards[piece_type_to_bitboard_index(side_to_move, PIECE_KNIGHT)])) { attacker_type = PIECE_KNIGHT; }
        else if ((bb = side_attackers & pos->bitboards[piece_type_to_bitboard_index(side_to_move, PIECE_BISHOP)])) { attacker_type = PIECE_BISHOP; }
        else if ((bb = side_attackers & pos->bitboards[piece_type_to_bitboard_index(side_to_move, PIECE_ROOK)]))   { attacker_type = PIECE_ROOK; }
        else if ((bb = side_attackers & pos->bitboards[piece_type_to_bitboard_index(side_to_move, PIECE_QUEEN)]))  { attacker_type = PIECE_QUEEN; }
        else if ((bb = side_attackers & pos->bitboards[piece_type_to_bitboard_index(side_to_move, PIECE_KING)]))   { attacker_type = PIECE_KING; }

        if (!bb) break;

        Square sq = (Square)__builtin_ctzll(bb);

        gain[d] = SEE_VALUES[next_victim] - gain[d - 1];
        
        // Remove the attacker
        occupied ^= (1ULL << sq);
        
        // Update attackers to reveal hidden sliders (X-rays)
        attackers = get_attackers(pos, to, occupied);
        
        next_victim = attacker_type;
        side_to_move = opposite(side_to_move);
    }

    // Minimax back-propagation
    while (--d > 0) {
        gain[d - 1] = gain[d - 1] < -gain[d] ? gain[d - 1] : -gain[d];
    }

    return gain[0];
}
