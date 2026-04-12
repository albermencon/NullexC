#include "functions/moves/MoveGeneration.h"
#include "functions/moves/MoveApplication.h"
#include "functions/attacks/AttackMasks.h"
#include "functions/rules/Check.h"
#include <assert.h>

// Thread-local storage
static _Thread_local Move      move_buffer[MAX_MOVES];

// gen_piece_moves — jumping + sliding pieces to target mask
static inline Move* gen_piece_moves(const Position* pos, Color us, PieceType pt,
    Move* moveList, Bitboard target)
{
    Bitboard pieces = pos->bitboards[piece_type_to_bitboard_index(us, pt)];
    while (pieces) {
        Square   from = pop_lsb(&pieces);
        Bitboard atk;
        switch (pt) {
        case PIECE_KNIGHT: atk = knight_attacks(from);                break;
        case PIECE_BISHOP: atk = bishop_attacks(from, pos->occupied); break;
        case PIECE_ROOK:   atk = rook_attacks(from, pos->occupied); break;
        case PIECE_QUEEN:  atk = queen_attacks(from, pos->occupied); break;
        case PIECE_KING:   atk = king_attacks(from);                  break;
        default: assert(0); atk = 0;
        }
        Bitboard moves = atk & target;
        while (moves) *moveList++ = MOVE_NORMAL(from, pop_lsb(&moves));
    }
    return moveList;
}

/*
    gen_pawn_moves_target — pawns restricted to a target mask
    includeQuiets = false for capture-only generation
    En passant is always emitted unconditionally (legality filter handles it)
*/
static inline Move* gen_pawn_moves_target(const Position* pos, Color us,
    Move* moveList, Bitboard target,
    bool includeQuiets)
{
    const Bitboard rank3 = (us == WHITE) ? 0x0000000000FF0000ULL : 0x0000FF0000000000ULL;
    const Bitboard rank7 = (us == WHITE) ? 0x00FF000000000000ULL : 0x000000000000FF00ULL;
    const Bitboard fileA = 0x0101010101010101ULL;
    const Bitboard fileH = 0x8080808080808080ULL;

    Bitboard pawns = pos->bitboards[piece_type_to_bitboard_index(us, PIECE_PAWN)];
    Bitboard empty = ~pos->occupied;
    Bitboard enemy = (us == WHITE) ? pos->blackPieces : pos->whitePieces;
    Bitboard nonPromo = pawns & ~rank7;
    Bitboard promo = pawns & rank7;

    // Quiet pushes restricted to target (used by evasions to block checker ray)
    if (includeQuiets) {
        Bitboard push1 = (us == WHITE) ? (nonPromo << 8) & empty & target
            : (nonPromo >> 8) & empty & target;
        Bitboard push2 = (us == WHITE) ? (((nonPromo << 8) & empty & rank3) << 8) & empty & target
            : (((nonPromo >> 8) & empty & rank3) >> 8) & empty & target;
        while (push1) { Square to = pop_lsb(&push1); *moveList++ = MOVE_NORMAL((us == WHITE) ? to - 8 : to + 8, to); }
        while (push2) { Square to = pop_lsb(&push2); *moveList++ = MOVE_NORMAL((us == WHITE) ? to - 16 : to + 16, to); }
    }

    // Captures restricted to target
    Bitboard capL = (us == WHITE) ? ((nonPromo & ~fileA) << 7) & enemy & target
        : ((nonPromo & ~fileH) >> 7) & enemy & target;
    Bitboard capR = (us == WHITE) ? ((nonPromo & ~fileH) << 9) & enemy & target
        : ((nonPromo & ~fileA) >> 9) & enemy & target;
    while (capL) { Square to = pop_lsb(&capL); *moveList++ = MOVE_NORMAL((us == WHITE) ? to - 7 : to + 7, to); }
    while (capR) { Square to = pop_lsb(&capR); *moveList++ = MOVE_NORMAL((us == WHITE) ? to - 9 : to + 9, to); }

    // Promotions restricted to target
    if (promo) {
        Bitboard ppush = includeQuiets
            ? ((us == WHITE) ? (promo << 8) & empty & target : (promo >> 8) & empty & target) : 0ULL;
        Bitboard pcapL = (us == WHITE) ? ((promo & ~fileA) << 7) & enemy & target : ((promo & ~fileH) >> 7) & enemy & target;
        Bitboard pcapR = (us == WHITE) ? ((promo & ~fileH) << 9) & enemy & target : ((promo & ~fileA) >> 9) & enemy & target;

#define EMIT_PROMOS(from, to) \
            *moveList++=MOVE_PROMOTION(from,to,PROMO_QUEEN);  \
            *moveList++=MOVE_PROMOTION(from,to,PROMO_KNIGHT); \
            *moveList++=MOVE_PROMOTION(from,to,PROMO_ROOK);   \
            *moveList++=MOVE_PROMOTION(from,to,PROMO_BISHOP);

        while (ppush) { Square to = pop_lsb(&ppush); EMIT_PROMOS((us == WHITE) ? to - 8 : to + 8, to) }
        while (pcapL) { Square to = pop_lsb(&pcapL); EMIT_PROMOS((us == WHITE) ? to - 7 : to + 7, to) }
        while (pcapR) { Square to = pop_lsb(&pcapR); EMIT_PROMOS((us == WHITE) ? to - 9 : to + 9, to) }
#undef EMIT_PROMOS
    }

    // En passant — always emitted, legality filter catches invalid ones
    if (pos->enPassantSquare != -1) {
        Square   ep = pos->enPassantSquare;
        Bitboard epB = 1ULL << ep;
        Bitboard cL = (us == WHITE) ? ((nonPromo & ~fileA) << 7) & epB : ((nonPromo & ~fileH) >> 7) & epB;
        Bitboard cR = (us == WHITE) ? ((nonPromo & ~fileH) << 9) & epB : ((nonPromo & ~fileA) >> 9) & epB;
        if (cL) *moveList++ = MOVE_EP((us == WHITE) ? ep - 7 : ep + 7, ep);
        if (cR) *moveList++ = MOVE_EP((us == WHITE) ? ep - 9 : ep + 9, ep);
    }

    return moveList;
}

/*
    generate_castling — only called when NOT in check
*/
static inline Move* generate_castling(const Position* pos, Color us, Move* moveList)
{
    const uint8_t kingside = (us == WHITE) ? 0x01 : 0x04;
    const uint8_t queenside = (us == WHITE) ? 0x02 : 0x08;
    if (!(pos->castlingRights & (kingside | queenside))) return moveList;

    const Square king = (us == WHITE) ? 4 : 60;
    const Color  them = opposite(us);

    if (pos->castlingRights & kingside) {
        const Bitboard mask = (us == WHITE) ? 0x60ULL : 0x6000000000000000ULL;
        if (!(pos->occupied & mask)
            && !isSquareAttacked(pos, king + 1, them)
            && !isSquareAttacked(pos, king + 2, them))
            *moveList++ = MOVE_CASTLE(king, king + 2);
    }
    if (pos->castlingRights & queenside) {
        const Bitboard mask = (us == WHITE) ? 0x0EULL : 0x0E00000000000000ULL;
        if (!(pos->occupied & mask)
            && !isSquareAttacked(pos, king - 1, them)
            && !isSquareAttacked(pos, king - 2, them))
            *moveList++ = MOVE_CASTLE(king, king - 2);
    }
    return moveList;
}

/*
    generate_evasions — single check only
    Generates king moves + non-pinned pieces that capture checker or block ray.
    Double check is handled separately in generate_legal_moves.
*/
static inline Move* generate_evasions(const Position* pos, Color us, Move* moveList,
    Bitboard checker)
{
    Color    them = opposite(us);
    Square   king = pos->kingSquare[us];
    Square   chkSq = (Square)lsb(checker);
    Bitboard pinned = pos->st->pinners[us];

    // King moves : all squares king attacks minus friendly pieces
    // Attacked-square filtering done in the legality pass (make/unmake)
    Bitboard kingMoves = king_attacks(king)
        & ~((us == WHITE) ? pos->whitePieces : pos->blackPieces);
    while (kingMoves) {
        Square to = pop_lsb(&kingMoves);
        *moveList++ = MOVE_NORMAL(king, to);
    }

    // Non pinned pieces capture checker OR block the ray between king and checker
    // Pinned pieces can never resolve a check.
    Bitboard blockMask = BetweenBB[king][chkSq] | checker;

    // Pawns (excluding pinned) — pushes + captures restricted to blockMask
    {
        const Bitboard rank3 = (us == WHITE) ? 0x0000000000FF0000ULL : 0x0000FF0000000000ULL;
        const Bitboard rank7 = (us == WHITE) ? 0x00FF000000000000ULL : 0x000000000000FF00ULL;
        const Bitboard fileA = 0x0101010101010101ULL;
        const Bitboard fileH = 0x8080808080808080ULL;
        Bitboard pawns = pos->bitboards[piece_type_to_bitboard_index(us, PIECE_PAWN)] & ~pinned;
        Bitboard empty = ~pos->occupied;
        Bitboard enemy = (us == WHITE) ? pos->blackPieces : pos->whitePieces;
        Bitboard nonPromo = pawns & ~rank7;
        Bitboard promo = pawns & rank7;

        Bitboard push1 = (us == WHITE) ? (nonPromo << 8) & empty & blockMask : (nonPromo >> 8) & empty & blockMask;
        Bitboard push2 = (us == WHITE) ? (((nonPromo << 8) & empty & rank3) << 8) & empty & blockMask
            : (((nonPromo >> 8) & empty & rank3) >> 8) & empty & blockMask;
        while (push1) { Square to = pop_lsb(&push1); *moveList++ = MOVE_NORMAL((us == WHITE) ? to - 8 : to + 8, to); }
        while (push2) { Square to = pop_lsb(&push2); *moveList++ = MOVE_NORMAL((us == WHITE) ? to - 16 : to + 16, to); }

        Bitboard capL = (us == WHITE) ? ((nonPromo & ~fileA) << 7) & enemy & blockMask : ((nonPromo & ~fileH) >> 7) & enemy & blockMask;
        Bitboard capR = (us == WHITE) ? ((nonPromo & ~fileH) << 9) & enemy & blockMask : ((nonPromo & ~fileA) >> 9) & enemy & blockMask;
        while (capL) { Square to = pop_lsb(&capL); *moveList++ = MOVE_NORMAL((us == WHITE) ? to - 7 : to + 7, to); }
        while (capR) { Square to = pop_lsb(&capR); *moveList++ = MOVE_NORMAL((us == WHITE) ? to - 9 : to + 9, to); }

        if (promo) {
            Bitboard ppush = (us == WHITE) ? (promo << 8) & empty & blockMask : (promo >> 8) & empty & blockMask;
            Bitboard pcapL = (us == WHITE) ? ((promo & ~fileA) << 7) & enemy & blockMask : ((promo & ~fileH) >> 7) & enemy & blockMask;
            Bitboard pcapR = (us == WHITE) ? ((promo & ~fileH) << 9) & enemy & blockMask : ((promo & ~fileA) >> 9) & enemy & blockMask;
#define EMIT_PROMOS(from,to) \
                *moveList++=MOVE_PROMOTION(from,to,PROMO_QUEEN); \
                *moveList++=MOVE_PROMOTION(from,to,PROMO_KNIGHT);\
                *moveList++=MOVE_PROMOTION(from,to,PROMO_ROOK);  \
                *moveList++=MOVE_PROMOTION(from,to,PROMO_BISHOP);
            while (ppush) { Square to = pop_lsb(&ppush); EMIT_PROMOS((us == WHITE) ? to - 8 : to + 8, to) }
            while (pcapL) { Square to = pop_lsb(&pcapL); EMIT_PROMOS((us == WHITE) ? to - 7 : to + 7, to) }
            while (pcapR) { Square to = pop_lsb(&pcapR); EMIT_PROMOS((us == WHITE) ? to - 9 : to + 9, to) }
#undef EMIT_PROMOS
        }

        // EP: only legal if captured pawn IS the checker OR the destination square blocks the check
        if (pos->enPassantSquare != -1) {
            Square ep = pos->enPassantSquare;
            Square epPwn = (us == WHITE) ? ep - 8 : ep + 8;

            // Check if captured pawn is checker, OR destination square intercepts the check ray
            if ((checker & (1ULL << epPwn)) || (blockMask & (1ULL << ep))) {
                Bitboard epB = 1ULL << ep;
                Bitboard cL = (us == WHITE) ? ((pawns & ~fileA) << 7) & epB : ((pawns & ~fileH) >> 7) & epB;
                Bitboard cR = (us == WHITE) ? ((pawns & ~fileH) << 9) & epB : ((pawns & ~fileA) >> 9) & epB;
                if (cL) *moveList++ = MOVE_EP((us == WHITE) ? ep - 7 : ep + 7, ep);
                if (cR) *moveList++ = MOVE_EP((us == WHITE) ? ep - 9 : ep + 9, ep);
            }
        }
    }

    // Non-pinned sliders + knights restricted to blockMask
#define GEN_EVADE(pt, atk_expr) do { \
        Bitboard pieces = pos->bitboards[piece_type_to_bitboard_index(us,pt)] & ~pinned; \
        while (pieces) { \
            Square from = pop_lsb(&pieces); \
            Bitboard mv = (atk_expr) & blockMask; \
            while (mv) *moveList++=MOVE_NORMAL(from,pop_lsb(&mv)); \
        } \
    } while(0)

    GEN_EVADE(PIECE_KNIGHT, knight_attacks(from));
    GEN_EVADE(PIECE_BISHOP, bishop_attacks(from, pos->occupied));
    GEN_EVADE(PIECE_ROOK, rook_attacks(from, pos->occupied));
    GEN_EVADE(PIECE_QUEEN, queen_attacks(from, pos->occupied));
#undef GEN_EVADE

    return moveList;
}

// generate_non_evasions — not in check, all pseudo-legal moves
static inline Move* generate_non_evasions(const Position* pos, Color us, Move* moveList)
{
    Bitboard target = ~((us == WHITE) ? pos->whitePieces : pos->blackPieces);

    moveList = gen_pawn_moves_target(pos, us, moveList, target, true);
    moveList = gen_piece_moves(pos, us, PIECE_KNIGHT, moveList, target);
    moveList = gen_piece_moves(pos, us, PIECE_BISHOP, moveList, target);
    moveList = gen_piece_moves(pos, us, PIECE_ROOK, moveList, target);
    moveList = gen_piece_moves(pos, us, PIECE_QUEEN, moveList, target);
    moveList = gen_piece_moves(pos, us, PIECE_KING, moveList, target);
    moveList = generate_castling(pos, us, moveList);
    return moveList;
}

static inline Move* filter_legal(const Position* pos, Move* begin, Move* end,
    Color us, Color them, Square king,
    Bitboard pinned, bool inCheck)
{
    // Precompute attacked squares once
    // Remove king from occupancy so sliders see through it.
    Bitboard occNoKing = pos->occupied ^ (1ULL << king);
    Bitboard attacked = 0ULL;
    {
        const Bitboard fileA = 0x0101010101010101ULL;
        const Bitboard fileH = 0x8080808080808080ULL;

        // Enemy pawn attacks (aggregate shift)
        Bitboard ep = pos->bitboards[piece_type_to_bitboard_index(them, PIECE_PAWN)];
        attacked |= (them == WHITE) ? ((ep & ~fileA) << 7) | ((ep & ~fileH) << 9)
            : ((ep & ~fileH) >> 7) | ((ep & ~fileA) >> 9);

        // Enemy knights
        Bitboard kn = pos->bitboards[piece_type_to_bitboard_index(them, PIECE_KNIGHT)];
        while (kn) attacked |= knight_attacks(pop_lsb(&kn));

        // Enemy bishops + queens (diagonal), using occNoKing
        Bitboard bq = pos->bitboards[piece_type_to_bitboard_index(them, PIECE_BISHOP)]
            | pos->bitboards[piece_type_to_bitboard_index(them, PIECE_QUEEN)];
        while (bq) attacked |= bishop_attacks(pop_lsb(&bq), occNoKing);

        // Enemy rooks + queens (straight), using occNoKing
        Bitboard rq = pos->bitboards[piece_type_to_bitboard_index(them, PIECE_ROOK)]
            | pos->bitboards[piece_type_to_bitboard_index(them, PIECE_QUEEN)];
        while (rq) attacked |= rook_attacks(pop_lsb(&rq), occNoKing);

        // Enemy king
        attacked |= king_attacks(pos->kingSquare[them]);
    }

    Position* p = (Position*)pos;
    Move* cur = begin;

    while (cur != end) {
        Square   from = move_from(*cur);
        Square   to = move_to(*cur);
        Bitboard fromBit = 1ULL << from;
        uint8_t  flag = move_special_flag(*cur);

        if (from == king) {
            // bit test
            if (attacked & (1ULL << to)) { *cur = *(--end); continue; }

        }
        else if (flag == EN_PASSANT) {
            // EP can expose discovered check along a rank.
            // Only case requiring make/unmake.
            StateInfo ep_state;
            makeMove(p, *cur, &ep_state);
            bool legal = !isSquareAttacked(p, p->kingSquare[us], them);
            unMakeMove(p, &ep_state);
            if (!legal) { *cur = *(--end); continue; }

        }
        else if (pinned & fromBit) {
            // pinned piece legal iff destination stays on pin ray.
            // LineBB[king][from] = full line through king and pinned piece (= pin ray).
            // Capturing the pinner is also on this line — correctly allowed.
            if (!((1ULL << to) & LineBB[king][from])) { *cur = *(--end); continue; }

        }
        // inCheck non-king non-EP non-pinned move came from generate_evasions
        // which already restricted to blockMask = BetweenBB[king][checker] | checker.
        // Resolves the check by definition — always legal, nothing to do.
        // Not-in-check non-king non-EP non-pinned → always legal (fast path).
        cur++;
    }
    return end;
}

int generate_legal_moves(const Position* pos, Move* moveList)
{
    Color    us = pos->whiteToMove ? WHITE : BLACK;
    Color    them = opposite(us);
    Square   king = pos->kingSquare[us];
    Bitboard checkers = pos->st->checkersBB;
    Bitboard pinned = pos->st->pinners[us];

    Move* end;

    if (checkers) {
        // Double check: only king moves possible
        if (checkers & (checkers - 1)) {
            end = moveList;
            Bitboard friendly = (us == WHITE) ? pos->whitePieces : pos->blackPieces;
            Bitboard kingMoves = king_attacks(king) & ~friendly;
            while (kingMoves) *end++ = MOVE_NORMAL(king, pop_lsb(&kingMoves));
            // All king moves need validation (walk into attack / slider ray)
            end = filter_legal(pos, moveList, end, us, them, king, 0ULL, true);
            return (int)(end - moveList);
        }
        // Single check: evasions
        end = generate_evasions(pos, us, moveList, checkers);
    }
    else {
        // Not in check
        end = generate_non_evasions(pos, us, moveList);
    }

    end = filter_legal(pos, moveList, end, us, them, king, pinned,
        checkers != 0ULL);
    return (int)(end - moveList);
}

int generate_capture_moves(const Position* pos, Move* moveList)
{
    Color    us = pos->whiteToMove ? WHITE : BLACK;
    Color    them = opposite(us);
    Square   king = pos->kingSquare[us];
    Bitboard checkers = pos->st->checkersBB;
    Bitboard pinned = pos->st->pinners[us];
    Bitboard enemy = (us == WHITE) ? pos->blackPieces : pos->whitePieces;

    // In check: only checker captures; otherwise all enemy captures
    Bitboard target = checkers ? checkers : enemy;

    Move* end = moveList;
    end = gen_pawn_moves_target(pos, us, end, target, false);
    end = gen_piece_moves(pos, us, PIECE_KNIGHT, end, target);
    end = gen_piece_moves(pos, us, PIECE_BISHOP, end, target);
    end = gen_piece_moves(pos, us, PIECE_ROOK, end, target);
    end = gen_piece_moves(pos, us, PIECE_QUEEN, end, target);
    end = gen_piece_moves(pos, us, PIECE_KING, end, enemy);

    end = filter_legal(pos, moveList, end, us, them, king, pinned,
        checkers != 0ULL);
    return (int)(end - moveList);
}

Bitboard get_capture_mask(const Position* pos, Square target_square, Color attacking_color)
{
    Bitboard captureMask = 0ULL;
    const Bitboard targetBit = 1ULL << target_square;
    const Bitboard* bb = pos->bitboards;
    Bitboard pieces;

#define CHECK_ATTACKERS(pt, atk_expr) do { \
        pieces = bb[piece_type_to_bitboard_index(attacking_color, pt)]; \
        while (pieces) { Square from=pop_lsb(&pieces); \
            if ((atk_expr) & targetBit) captureMask |= 1ULL<<from; } \
    } while(0)

    pieces = bb[piece_type_to_bitboard_index(attacking_color, PIECE_PAWN)];
    while (pieces) {
        Square from = pop_lsb(&pieces);
        if (pawn_attacks(from, attacking_color) & targetBit) captureMask |= 1ULL << from;
    }
    CHECK_ATTACKERS(PIECE_KNIGHT, knight_attacks(from));
    CHECK_ATTACKERS(PIECE_BISHOP, bishop_attacks(from, pos->occupied));
    CHECK_ATTACKERS(PIECE_ROOK, rook_attacks(from, pos->occupied));
    CHECK_ATTACKERS(PIECE_QUEEN, queen_attacks(from, pos->occupied));
    CHECK_ATTACKERS(PIECE_KING, king_attacks(from));
#undef CHECK_ATTACKERS

    return captureMask;
}
