#ifndef ZOBRIST_HASH_H
#define ZOBRIST_HASH_H

#include <stdint.h>
#include <stdbool.h>

#include "core/types.h"
#include "data/Position.h"

typedef struct {
    int rookFrom;
    int rookTo;
} CastlingPositions;

void zobristHash_init();

uint64_t compute_hash(const uint64_t *bitboards, bool whiteToMove, uint8_t castlingRights, int enPassantTarget);

uint64_t update_hash_normal(uint64_t hash, int movingPiece, int capturedPiece, const Position *oldPos, int enPassant, uint8_t newCastling, Move move);
uint64_t update_hash_promotion(uint64_t hash, int pawnIndex, int promoIndex, int capturedPiece, const Position *oldPos, uint8_t newCastling, Move move);
uint64_t update_hash_en_passant(uint64_t hash, int movingPawn, int capturedPawn, int capturedSq, const Position *oldPos, Move move);
uint64_t update_hash_castling(uint64_t hash, int kingIndex, int rookIndex, CastlingPositions positions, const Position *oldPos, uint8_t newCastling, Move move);
uint64_t update_hash_null_move(uint64_t hash, const Position *oldPos);

#endif // ZOBRIST_HASH_H
