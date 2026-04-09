#ifndef MOVE_APPLICATION_H
#define MOVE_APPLICATION_H

#include <stdint.h>
#include <stdbool.h>

#include "core/types.h"
#include "data/Position.h"

#include "functions/hash/ZobristHash.h"

void makeMove(Position* pos, Move move, StateInfo* state);
void unMakeMove(Position* pos, const StateInfo* state);

#endif // MOVE_APPLICATION_H
