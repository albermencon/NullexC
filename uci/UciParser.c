#include "UciParser.h"
#include "UciUtils.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "data/Position.h"
#include "data/TranspositionTable.h"
#include "functions/moves/MoveGeneration.h"
#include "functions/moves/MoveApplication.h"
#include "functions/search/Search.h"
#include "functions/benchmark/Perft.h"
#include "functions/hash/ZobristHash.h"

extern TranspositionTable TT;

void uci_state_init(UciState* state) {
    parse_fen(&state->pos, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    state->game_ply = 0;
    position_setup_root(&state->pos, &state->game_states[state->game_ply]);
}

void parse_fen(Position* pos, const char* fen) {
    uint64_t bitboards[12] = {0};
    char fen_copy[256];
    strncpy(fen_copy, fen, 255);
    
    char* board_part = strtok(fen_copy, " ");
    char* turn_part  = strtok(NULL, " ");
    char* cast_part  = strtok(NULL, " ");
    char* ep_part    = strtok(NULL, " ");
    char* half_part  = strtok(NULL, " ");
    char* full_part  = strtok(NULL, " ");

    int rank = 7, file = 0;
    for (int i = 0; board_part[i] != '\0'; i++) {
        char c = board_part[i];
        if (c == '/') { rank--; file = 0; }
        else if (isdigit(c)) { file += (c - '0'); }
        else {
            int sq = rank * 8 + file;
            uint64_t bit = 1ULL << sq;
            switch (c) {
                case 'P': bitboards[0] |= bit; break;
                case 'N': bitboards[1] |= bit; break;
                case 'B': bitboards[2] |= bit; break;
                case 'R': bitboards[3] |= bit; break;
                case 'Q': bitboards[4] |= bit; break;
                case 'K': bitboards[5] |= bit; break;
                case 'p': bitboards[6] |= bit; break;
                case 'n': bitboards[7] |= bit; break;
                case 'b': bitboards[8] |= bit; break;
                case 'r': bitboards[9] |= bit; break;
                case 'q': bitboards[10] |= bit; break;
                case 'k': bitboards[11] |= bit; break;
            }
            file++;
        }
    }

    bool whiteToMove = (turn_part[0] == 'w');

    uint8_t castling = 0;
    if (strchr(cast_part, 'K')) castling |= 0x01;
    if (strchr(cast_part, 'Q')) castling |= 0x02;
    if (strchr(cast_part, 'k')) castling |= 0x04;
    if (strchr(cast_part, 'q')) castling |= 0x08;

    short epSq = (ep_part[0] == '-') ? -1 : (short)uci_to_sq(ep_part);

    short halfClock = (half_part) ? (short)atoi(half_part) : 0;
    int fullMove = (full_part) ? atoi(full_part) : 1;

    uint64_t hash = compute_hash(bitboards, whiteToMove, castling, epSq);
    Position_init(pos, bitboards, whiteToMove, castling, epSq, halfClock, fullMove, hash);
}

Move parse_move(Position* pos, const char* move_str) {
    Move moves[MAX_MOVES];
    int moveCount = generate_legal_moves(pos, moves);
    for (int i = 0; i < moveCount; i++) {
        char buf[6];
        int from = move_from(moves[i]), to = move_to(moves[i]);
        sprintf(buf, "%c%c%c%c", 'a'+(from%8), '1'+(from/8), 'a'+(to%8), '1'+(to/8));
        
        if (move_special_flag(moves[i]) == PROMOTION) {
            PieceType pt = get_promotion_piece_type(moves[i]);
            const char* promo_map = " nbrq";
            buf[4] = promo_map[pt];
            buf[5] = '\0';
        } else {
            buf[4] = '\0';
        }

        if (strcmp(move_str, buf) == 0) return moves[i];
    }
    return move_null();
}

void parse_position(UciState* state, char* line) {
    char* pos_ptr = strstr(line, "startpos");
    char* fen_ptr = strstr(line, "fen");

    if (pos_ptr) {
        parse_fen(&state->pos, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    } else if (fen_ptr) {
        parse_fen(&state->pos, fen_ptr + 4);
    } else {
        parse_fen(&state->pos, "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1");
    }

    state->game_ply = 0;
    position_setup_root(&state->pos, &state->game_states[state->game_ply]);

    char* moves_ptr = strstr(line, "moves");
    if (moves_ptr) {
        moves_ptr += 6;
        char* saveptr;
        char* token = strtok_r(moves_ptr, " ", &saveptr);
        while (token) {
            Move m = parse_move(&state->pos, token);
            if (m != move_null() && state->game_ply < MAX_GAME_PLY - 1) {
                state->game_ply++;
                makeMove(&state->pos, m, &state->game_states[state->game_ply]);
            }
            token = strtok_r(NULL, " ", &saveptr);
        }
    }
}

void parse_go(Position* pos, char* line) {
    int wtime = 0, btime = 0, winc = 0, binc = 0, movetime = 0, depth = -1;
    char* ptr;
    if ((ptr = strstr(line, "wtime"))) wtime = atoi(ptr + 6);
    if ((ptr = strstr(line, "btime"))) btime = atoi(ptr + 6);
    if ((ptr = strstr(line, "winc")))  winc = atoi(ptr + 5);
    if ((ptr = strstr(line, "binc")))  binc = atoi(ptr + 5);
    if ((ptr = strstr(line, "movetime"))) movetime = atoi(ptr + 9);
    if ((ptr = strstr(line, "depth"))) depth = atoi(ptr + 6);
    
    if (strstr(line, "perft")) {
        perft_divide(pos, atoi(strstr(line, "perft") + 6));
        return;
    }

    uint64_t allocated = 0;
    if (movetime > 0) allocated = movetime;
    else if (wtime > 0 || btime > 0) {
        int t = pos->whiteToMove ? wtime : btime;
        int i = pos->whiteToMove ? winc : binc;
        allocated = (t / 20) + (i / 2);

        if (t > 50) {
            if (allocated > (uint64_t)(t - 50)) {
                allocated = t - 50;
            }
        } else {
            allocated = 1;
        }
    }

    tt_newSearch(&TT);
    Move best = (allocated > 0) ? search_iterative(pos, allocated) 
                                : search_iterative_depth(pos, depth > 0 ? depth : 6);

    printf("bestmove ");
    print_uci_move(best);
    printf("\n");
}
