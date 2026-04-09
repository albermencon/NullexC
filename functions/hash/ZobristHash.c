#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

#include "ZobristHash.h"
#include "core/bitboard.h"

#define NUM_PIECES 12
#define NUM_SQUARES 64
#define NUM_CASTLING 16
#define NUM_FILES 8

static uint64_t PIECE_SQUARE_KEYS[NUM_PIECES][NUM_SQUARES];
static uint64_t WHITE_TO_MOVE_KEY;
static uint64_t CASTLING_KEYS[NUM_CASTLING];
static uint64_t EN_PASSANT_KEYS[NUM_FILES];

#define SQUARE_TO_FILE(sq) ((sq) & 7)

// SplitMix64 PRNG
static uint64_t splitmix64_state;

static uint64_t splitmix64_next(void) {
    uint64_t z = (splitmix64_state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

void zobristHash_init() {
    splitmix64_state = 1070372ULL;

    for (int p = 0; p < NUM_PIECES; p++)
    {
        for (int s = 0; s < NUM_SQUARES; s++)
        {
            PIECE_SQUARE_KEYS[p][s] = splitmix64_next();
        }
    }

    WHITE_TO_MOVE_KEY = splitmix64_next();

    for (int i = 0; i < NUM_CASTLING; i++) {
        CASTLING_KEYS[i] = splitmix64_next();
    }

    for (int i = 0; i < NUM_FILES; i++) {
        EN_PASSANT_KEYS[i] = splitmix64_next();
    }
}

static uint64_t hash_pieces(const uint64_t *bitboards) {
    uint64_t hash = 0ULL;
    for (int piece = 0; piece < NUM_PIECES; piece++) {
        uint64_t bb = bitboards[piece];
        while (bb) {
            int sq = lsb(bb);
            hash ^= PIECE_SQUARE_KEYS[piece][sq];
            bb &= bb - 1;
        }
    }
    return hash;
}

uint64_t compute_hash(const uint64_t *bitboards, bool whiteToMove, uint8_t castlingRights, int enPassantTarget) {
    uint64_t hash = hash_pieces(bitboards);
    if (whiteToMove) hash ^= WHITE_TO_MOVE_KEY;
    hash ^= CASTLING_KEYS[castlingRights & 0xF];
    if (enPassantTarget != -1) {
        int file = enPassantTarget % 8;
        hash ^= EN_PASSANT_KEYS[file];
    }
    return hash;
}

static uint64_t update_en_passant(uint64_t hash, int oldEp, int newEp) {
    if (oldEp != -1) hash ^= EN_PASSANT_KEYS[SQUARE_TO_FILE(oldEp)];
    if (newEp != -1) hash ^= EN_PASSANT_KEYS[SQUARE_TO_FILE(newEp)];
    return hash;
}

uint64_t update_hash_normal(uint64_t hash, int movingPiece, int capturedPiece, const Position *oldPos, int enPassant, uint8_t newCastling, Move move) {
    int from = move_from(move);
    int to = move_to(move);

    hash ^= PIECE_SQUARE_KEYS[movingPiece][from];
    hash ^= PIECE_SQUARE_KEYS[movingPiece][to];
    if (capturedPiece != -1) hash ^= PIECE_SQUARE_KEYS[capturedPiece][to];

    hash ^= WHITE_TO_MOVE_KEY;
    hash ^= CASTLING_KEYS[oldPos->castlingRights & 0xF];
    hash ^= CASTLING_KEYS[newCastling & 0xF];
    return update_en_passant(hash, oldPos->enPassantSquare, enPassant);
}

uint64_t update_hash_promotion(uint64_t hash, int pawnIndex, int promoIndex, int capturedPiece, const Position *oldPos, uint8_t newCastling, Move move) {
    int from = move_from(move);
    int to = move_to(move);

    hash ^= PIECE_SQUARE_KEYS[pawnIndex][from];
    hash ^= PIECE_SQUARE_KEYS[promoIndex][to];
    if (capturedPiece != -1) hash ^= PIECE_SQUARE_KEYS[capturedPiece][to];

    hash ^= WHITE_TO_MOVE_KEY;
    hash ^= CASTLING_KEYS[oldPos->castlingRights & 0xF];
    hash ^= CASTLING_KEYS[newCastling & 0xF];
    return update_en_passant(hash, oldPos->enPassantSquare, -1);
}

uint64_t update_hash_en_passant(uint64_t hash, int movingPawn, int capturedPawn, int capturedSq, const Position *oldPos, Move move) {
    int from = move_from(move);
    int to = move_to(move);

    hash ^= PIECE_SQUARE_KEYS[movingPawn][from];
    hash ^= PIECE_SQUARE_KEYS[movingPawn][to];
    hash ^= PIECE_SQUARE_KEYS[capturedPawn][capturedSq];

    hash ^= WHITE_TO_MOVE_KEY;
    return update_en_passant(hash, oldPos->enPassantSquare, -1);
}

uint64_t update_hash_castling(uint64_t hash, int kingIndex, int rookIndex, CastlingPositions positions, const Position *oldPos, uint8_t newCastling, Move move) {
    int kingFrom = move_from(move);
    int kingTo = move_to(move);

    hash ^= PIECE_SQUARE_KEYS[kingIndex][kingFrom];
    hash ^= PIECE_SQUARE_KEYS[kingIndex][kingTo];
    hash ^= PIECE_SQUARE_KEYS[rookIndex][positions.rookFrom];
    hash ^= PIECE_SQUARE_KEYS[rookIndex][positions.rookTo];

    hash ^= WHITE_TO_MOVE_KEY;
    hash ^= CASTLING_KEYS[oldPos->castlingRights & 0xF];
    hash ^= CASTLING_KEYS[newCastling & 0xF];
    return update_en_passant(hash, oldPos->enPassantSquare, -1);
}

uint64_t update_hash_null_move(uint64_t hash, const Position *oldPos) {
    hash ^= WHITE_TO_MOVE_KEY;
    if (oldPos->enPassantSquare != -1) hash ^= EN_PASSANT_KEYS[oldPos->enPassantSquare & 7];
    return hash;
}