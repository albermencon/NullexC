#include "functions/benchmark/Perft.h"
#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "functions/moves/MoveGeneration.h"
#include "functions/moves/MoveApplication.h"
#include "functions/search/Time.h"

// forward declarations
static void move_to_uci(Move move, char *buf);
static void square_to_string(int sq, char *buf);

static void square_to_string(int sq, char *buf) {
    int file = sq % 8;
    int rank = sq / 8;
    buf[0] = 'a' + file;
    buf[1] = '1' + rank;
    buf[2] = '\0';
}

static void move_to_uci(Move move, char *buf) {
    int from = move_from(move);
    int to   = move_to(move);

    char fbuf[3], tbuf[3];
    square_to_string(from, fbuf);
    square_to_string(to, tbuf);

    // base "e2e4"
    buf[0] = fbuf[0];
    buf[1] = fbuf[1];
    buf[2] = tbuf[0];
    buf[3] = tbuf[1];
    buf[4] = '\0';

    // append promotion letter if promotion move
    if (move_special_flag(move) == PROMOTION) {
        PieceType pt = get_promotion_piece_type(move);
        char pc = '?';
        switch (pt) {
            case PIECE_QUEEN:  pc = 'q'; break;
            case PIECE_ROOK:   pc = 'r'; break;
            case PIECE_BISHOP: pc = 'b'; break;
            case PIECE_KNIGHT: pc = 'n'; break;
            default:           pc = '?'; break;
        }
        buf[4] = pc;
        buf[5] = '\0';
    }
}

uint64_t perft(Position* pos, int depth)
{
    if (depth <= 0) return 1;
        
    Move moves[MAX_MOVES];
    int moveCount = generate_legal_moves(pos, moves);

    if (depth == 1)
        return (uint64_t)moveCount;

    uint64_t nodes = 0;
    StateInfo state;

    if (depth == 2) {
        Move innerMoves[MAX_MOVES];
        for (int i = 0; i < moveCount; i++) {
            makeMove(pos, moves[i], &state);
            nodes += (uint64_t)generate_legal_moves(pos, innerMoves);
            unMakeMove(pos, &state);
        }
        return nodes;
    }

    for (int i = 0; i < moveCount; i++)
    {
        makeMove(pos, moves[i], &state);
        nodes += perft(pos, depth - 1);
        unMakeMove(pos, &state);
    }

    return nodes;
}

uint64_t perft_divide(const Position* pos, int depth) {
    if (depth < 1) return 0;

    Move moves[MAX_MOVES];
    // cast to a non const position temporary
    int moveCount = generate_legal_moves((Position*)pos, moves);

    uint64_t total = 0;
    printf("Perft divide depth %d\n", depth);
    uint64_t start_time = get_time_ms();
    for (int i = 0; i < moveCount; ++i) {
        // Make a local copy of the root position, apply the root move on the copy,
        // then call perft on depth-1
        Position tmp;
        memcpy(&tmp, pos, sizeof(Position));

        StateInfo state;
        makeMove(&tmp, moves[i], &state);

        uint64_t nodes = perft(&tmp, depth - 1);

        // restore tmp
        unMakeMove(&tmp, &state);

        // format move and print count
        char mstr[8];
        move_to_uci(moves[i], mstr);
        printf("%s : %llu\n", mstr, (unsigned long long)nodes);

        total += nodes;
    }
    uint64_t end_time = get_time_ms();
    double time_taken = (double)(end_time - start_time);

    printf("Total nodes: %llu\nTime: %0.2f ms\n", (unsigned long long)total, time_taken);
    return total;
}
