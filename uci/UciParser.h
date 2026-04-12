#ifndef UCI_PARSER_H
#define UCI_PARSER_H

#include "data/Position.h"
#include "core/types.h"

#define MAX_GAME_PLY 2048

typedef struct {
    Position pos;
    StateInfo game_states[MAX_GAME_PLY];
    int game_ply;
} UciState;

void uci_state_init(UciState* state);
void parse_fen(Position* pos, const char* fen);
Move parse_move(Position* pos, const char* move_str);
void parse_position(UciState* state, char* line);
void parse_go(Position* pos, char* line);

#endif // UCI_PARSER_H
