#include "functions/moves/MoveApplication.h"
#include "functions/rules/Check.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

inline static uint8_t update_castling_rights(const Position* position, Move move, int movingPiece, int capturedPiece);
inline static short update_en_passant_square(Move move, int movingPiece);

void makeMove(Position *pos, Move move, StateInfo *state)
{
    Color movingColor = pos->whiteToMove ? WHITE : BLACK;
    Square from = square_of(move_from(move));
    Square to   = square_of(move_to(move));

    int movingPiece = Position_PieceAt(pos, from);
    int capturedPiece = Position_PieceAt(pos, to);

    uint8_t moveFlag = move_special_flag(move);
    bool isEnPassant = moveFlag == EN_PASSANT;
    bool isPromotion = moveFlag == PROMOTION;
    bool isCastling  = moveFlag == CASTLING;

    // Save state for unmake
    state->move = move;
    state->movingPiece = movingPiece;
    state->capturedPiece = capturedPiece;
    state->prevCastling = pos->castlingRights;
    state->prevEPSq = pos->enPassantSquare;
    state->prevHalfMove = pos->halfmoveClock;
    state->prevZobrist = pos->zobristHash;
    state->prev = pos->st;   // link into chain

    // Half-move clock
    pos->halfmoveClock = (is_pawn(movingPiece) || capturedPiece != -1)
        ? 0 : pos->halfmoveClock + 1;

    uint8_t newCastlingRights = update_castling_rights(pos, move, movingPiece, capturedPiece);
    short   newEnPassant      = update_en_passant_square(move, movingPiece);

    // Now update the Zobrist hash based on move type
    if (isCastling) {
        // Handle castling
        CastlingPositions castlingPos;
        if (to > from) { // kingside
            castlingPos.rookFrom = from + 3;
            castlingPos.rookTo = from + 1;
        } else { // queenside
            castlingPos.rookFrom = from - 4;
            castlingPos.rookTo = from - 1;
        }
        
        int kingIndex = movingPiece;
        int rookIndex = piece_type_to_bitboard_index(movingColor, PIECE_ROOK);
        
        pos->zobristHash = update_hash_castling(pos->zobristHash, kingIndex, rookIndex, 
                                                castlingPos, pos, newCastlingRights, move);
        
        // Move the pieces
        remove_piece_bb(pos, movingPiece, from);
        add_piece_bb(pos, movingPiece, to);
        remove_piece_bb(pos, rookIndex, castlingPos.rookFrom);
        add_piece_bb(pos, rookIndex, castlingPos.rookTo);
    }
    else if (isEnPassant) {
        // Handle en passant
        Square epCapturedSquare = (movingColor == WHITE) ? to - 8 : to + 8;
        int capturedPawnIndex = piece_type_to_bitboard_index(opposite(movingColor), PIECE_PAWN);
        
        pos->zobristHash = update_hash_en_passant(pos->zobristHash, movingPiece, 
                                                  capturedPawnIndex, epCapturedSquare, pos, move);
        
        // Move the pieces
        remove_piece_bb(pos, movingPiece, from);
        add_piece_bb(pos, movingPiece, to);
        remove_piece_bb(pos, capturedPawnIndex, epCapturedSquare);
        
        state->capturedPiece = capturedPawnIndex; // Update for unmake
    }
    else if (isPromotion) {
        // Handle promotion
        PieceType promoType = move_promo_type(move);
        int promoIndex = piece_type_to_bitboard_index(movingColor, promoType);
        int pawnIndex = movingPiece;
        
        pos->zobristHash = update_hash_promotion(pos->zobristHash, pawnIndex, promoIndex, 
                                                 capturedPiece, pos, newCastlingRights, move);
        
        // Move the pieces
        remove_piece_bb(pos, movingPiece, from);
        if (capturedPiece != -1) {
            remove_piece_bb(pos, capturedPiece, to);
        }
        add_piece_bb(pos, promoIndex, to);
    }
    else {
        // Normal move
        pos->zobristHash = update_hash_normal(pos->zobristHash, movingPiece, capturedPiece, 
                                              pos, newEnPassant, newCastlingRights, move);
        
        // Move the pieces
        remove_piece_bb(pos, movingPiece, from);
        if (capturedPiece != -1) {
            remove_piece_bb(pos, capturedPiece, to);
        }
        add_piece_bb(pos, movingPiece, to);
    }

    // Update position state
    pos->castlingRights = newCastlingRights;
    pos->enPassantSquare = newEnPassant;
    pos->whiteToMove = !pos->whiteToMove;

    // Cache checkers + pinned for the side now to move (read-only in generate_legal_moves)
    Color newUs = pos->whiteToMove ? WHITE : BLACK; // side to move next
    Color newThem = opposite(newUs);
    Square newKing = pos->kingSquare[newUs];

    state->checkersBB = get_checkers(pos, newUs, newKing, newThem);
    state->pinners[newUs] = compute_pinned(pos, newUs, pos->kingSquare[newUs]);
    state->pinners[newThem] = compute_pinned(pos, newThem, pos->kingSquare[newThem]);

    pos->st = state;  // advance the state pointer
}

void unMakeMove(Position *pos, const StateInfo *state)
{
    Move move = state->move;
    Square from = square_of(move_from(move));
    Square to   = square_of(move_to(move));
    int movingPiece = state->movingPiece;
    int capturedPiece = state->capturedPiece;

    uint8_t moveFlag = move_special_flag(move);
    bool isEnPassant = moveFlag == EN_PASSANT;
    bool isPromotion = moveFlag == PROMOTION;
    bool isCastling  = moveFlag == CASTLING;
    
    // Flip side back first to get correct moving color
    pos->whiteToMove = !pos->whiteToMove;
    Color movingColor = pos->whiteToMove ? WHITE : BLACK;

    if (isCastling) {
        // Undo castling
        remove_piece_bb(pos, movingPiece, to);
        add_piece_bb(pos, movingPiece, from);
        bool    kingside = to > from;
        Square  rookFrom = kingside ? from + 3 : from - 4;
        Square  rookTo   = kingside ? from + 1 : from - 1;
        int     rookPiece= piece_type_to_bitboard_index(movingColor, PIECE_ROOK);
        remove_piece_bb(pos, rookPiece, rookTo);
        add_piece_bb(pos, rookPiece, rookFrom);
    }
    else if (isEnPassant) {
        // Undo en passant
        remove_piece_bb(pos, movingPiece, to);
        add_piece_bb(pos, movingPiece, from);
        
        Square epCapturedSquare = (movingColor == WHITE) ? (to - 8) : (to + 8);
        add_piece_bb(pos, capturedPiece, epCapturedSquare);
    }
    else if (isPromotion) {
        // Undo promotion
        int pieceAtTo = Position_PieceAt(pos, to);
        remove_piece_bb(pos, pieceAtTo, to);
        add_piece_bb(pos, movingPiece, from); // Restore original pawn
        
        if (capturedPiece != -1) {
            add_piece_bb(pos, capturedPiece, to);
        }
    }
    else {
        // Undo normal move
        remove_piece_bb(pos, movingPiece, to);
        add_piece_bb(pos, movingPiece, from);
        
        if (capturedPiece != -1) {
            add_piece_bb(pos, capturedPiece, to);
        }
    }

    // Restore all state
    pos->castlingRights = state->prevCastling;
    pos->enPassantSquare = state->prevEPSq;
    pos->halfmoveClock = state->prevHalfMove;
    pos->zobristHash = state->prevZobrist;  // Simply restore the saved hash

    pos->st = state->prev;  // restore previous state pointer
}

void makeNullMove(Position* pos, StateInfo* state)
{
    state->move = move_null();
    state->movingPiece = -1;
    state->capturedPiece = -1;
    state->prevCastling = pos->castlingRights;
    state->prevEPSq = pos->enPassantSquare;
    state->prevHalfMove = pos->halfmoveClock;
    state->prevZobrist = pos->zobristHash;
    state->prev = pos->st;

    pos->zobristHash = update_hash_null_move(pos->zobristHash, pos);
    pos->enPassantSquare = -1;
    pos->whiteToMove = !pos->whiteToMove;
    pos->halfmoveClock++;

    Color  newUs = pos->whiteToMove ? WHITE : BLACK;
    Color  newThem = opposite(newUs);
    Square newKing = pos->kingSquare[newUs];

    state->checkersBB = get_checkers(pos, newUs, newKing, newThem);
    state->pinners[newUs] = compute_pinned(pos, newUs, pos->kingSquare[newUs]);
    state->pinners[newThem] = compute_pinned(pos, newThem, pos->kingSquare[newThem]);

    pos->st = state;
}

void unMakeNullMove(Position *pos, const StateInfo *state)
{
    // Restore all state
    pos->whiteToMove = !pos->whiteToMove;
    pos->castlingRights = state->prevCastling;
    pos->enPassantSquare = state->prevEPSq;
    pos->halfmoveClock = state->prevHalfMove;
    pos->zobristHash = state->prevZobrist;
    pos->st = state->prev;
}

static uint8_t update_castling_rights(const Position* position, Move move, int movingPiece, int capturedPiece)
{
    uint8_t rights = position->castlingRights;
    Square from = move_from(move);
    Square to   = move_to(move);
    Color movingColor = piece_index_color(movingPiece);

    // King moves -> remove both castling rights for this color
    if (is_king(movingPiece)) {
        if (movingColor == WHITE) {
            rights &= ~(0x01 | 0x02); // remove white kingside & queenside
        } else {
            rights &= ~(0x04 | 0x08); // remove black kingside & queenside
        }
    }

    // Rook moves -> remove corresponding side
    if (is_rook(movingPiece)) {
        // White rooks
        if (movingColor == WHITE) {
            if (from == 0) rights &= ~0x02;   // a1 rook → remove white queenside
            else if (from == 7) rights &= ~0x01; // h1 rook → remove white kingside
        } 
        // Black rooks
        else {
            if (from == 56) rights &= ~0x08; // a8 rook → remove black queenside
            else if (from == 63) rights &= ~0x04; // h8 rook → remove black kingside
        }
    }

    // Rook captured → remove corresponding side
    if (capturedPiece != -1 && is_rook(capturedPiece)) {
        Color capturedColor = piece_index_color(capturedPiece);
        if (capturedColor == WHITE) {
            if (to == 0) rights &= ~0x02;   // a1 rook captured → remove white queenside
            else if (to == 7) rights &= ~0x01; // h1 rook captured → remove white kingside
        } else {
            if (to == 56) rights &= ~0x08; // a8 rook captured → remove black queenside
            else if (to == 63) rights &= ~0x04; // h8 rook captured → remove black kingside
        }
    }

    return rights;
}

static short update_en_passant_square(Move move, int movingPiece)
{
    // Only pawns can create en passant
    if (!is_pawn(movingPiece)) {
        return -1;
    }

    Square from = move_from(move);
    Square to   = move_to(move);

    int rank_from = square_rank(from);
    int rank_to   = square_rank(to);

    // If pawn moved two squares, activate en passant
    if (abs(rank_to - rank_from) == 2) {
        // The en passant square is the "intermediate" square
        return (from + to) / 2;
    }

    return -1;
}