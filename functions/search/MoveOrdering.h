#ifndef MOVE_ORDERING_H
#define MOVE_ORDERING_H

#include "core/types.h"
#include "data/Position.h"

extern _Thread_local Move killer_moves[MAX_PLY][2];
extern _Thread_local int history_table[COLOR_NB][PIECE_NB][SQUARE_NB];

void order_moves(const Position *pos, Move ttMove, Move *moves, int count,
                 int ply);

#endif // MOVE_ORDERING_H
