#ifndef MOVE_APPLICATION_H
#define MOVE_APPLICATION_H

#include <stdint.h>
#include <stdbool.h>

#include "core/types.h"
#include "data/Position.h"

void makeMove(Position* pos, Move move, StateInfo* state);
void unMakeMove(Position* pos, const StateInfo* state);

void makeNullMove(Position* pos, StateInfo* state);
void unMakeNullMove(Position* pos, const StateInfo* state);

#endif // MOVE_APPLICATION_H
