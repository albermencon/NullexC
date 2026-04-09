#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "functions/hash/ZobristHash.h"
#include "functions/attacks/AttackMasks.h"
#include "functions/benchmark/Perft.h"
#include "data/Position.h"
#include "functions/moves/MoveApplication.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

#define NUM_BITBOARDS 12
#define CHESS_TILES 64

enum {
    wP = 0, wN, wB, wR, wQ, wK,
    bP, bN, bB, bR, bQ, bK
};

static void standPos(Position* pos);
static uint64_t* standBitboard(uint64_t* bitboard);

int main() {
    setvbuf(stdout, NULL, _IONBF, 0);
    initialize_attack_masks();
    init_line_between_bb();
    zobristHash_init();

    // Add Evaluation Function

    Position pos; 
    StateInfo root_state;

    standPos(&pos);
    position_setup_root(&pos, &root_state);  // pos->st now valid

    int MaxDepth = 10;
    for (int depth = 1; depth <= MaxDepth; depth++)
    {
#ifdef _WIN32

        LARGE_INTEGER frequency, start, end;
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&start);

        uint64_t nodeCount = perft(&pos, depth);

        QueryPerformanceCounter(&end);
        double milliseconds = (double)(end.QuadPart - start.QuadPart) * 1000.0 / (double)frequency.QuadPart;

#else

        struct timespec start, end;
        clock_gettime(CLOCK_MONOTONIC, &start);

        uint64_t nodeCount = perft(&pos, depth);

        clock_gettime(CLOCK_MONOTONIC, &end);

        double milliseconds =
            (end.tv_sec - start.tv_sec) * 1000.0 +
            (end.tv_nsec - start.tv_nsec) / 1000000.0;

#endif

        printf("Depth %d -> %llu | Elapsed time: %.3f ms\n", depth, nodeCount, milliseconds);
    }
    
    free_attack_tables();
    return 0;
}

static void standPos(Position* pos) {
    uint64_t bitboard[12];
    standBitboard(bitboard);

    const bool whiteToMove = true;
    const uint8_t castlingRights = 0b1111;
    const short enPassantSquare = -1;
    const short halfMoveCounter = 0;
    const int fullMoveCounter = 0;
    const uint64_t zobristHash = compute_hash(bitboard, whiteToMove, castlingRights, enPassantSquare);

    Position_init(pos, bitboard, whiteToMove, castlingRights, enPassantSquare, halfMoveCounter, fullMoveCounter, zobristHash);
}

static uint64_t* standBitboard(uint64_t* bitboard) {
    const char chessBoard[CHESS_TILES + 1] = "RNBQKBNRPPPPPPPP                                pppppppprnbqkbnr";

    // Limpia los bitboards por si acaso
    for (int i = 0; i < NUM_BITBOARDS; i++) {
        bitboard[i] = 0ULL;
    }

    for (int i = 0; i < CHESS_TILES; i++) {
        uint64_t bit = 1ULL << i;
        switch (chessBoard[i]) {
            case 'P': bitboard[wP] |= bit; break;
            case 'N': bitboard[wN] |= bit; break;
            case 'B': bitboard[wB] |= bit; break;
            case 'R': bitboard[wR] |= bit; break;
            case 'Q': bitboard[wQ] |= bit; break;
            case 'K': bitboard[wK] |= bit; break;
            case 'p': bitboard[bP] |= bit; break;
            case 'n': bitboard[bN] |= bit; break;
            case 'b': bitboard[bB] |= bit; break;
            case 'r': bitboard[bR] |= bit; break;
            case 'q': bitboard[bQ] |= bit; break;
            case 'k': bitboard[bK] |= bit; break;
            default: break;
        }
    }

    return bitboard;
}
