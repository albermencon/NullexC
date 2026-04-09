#pragma once
#include <stdint.h>
#include "data/Position.h"

uint64_t perft(Position* pos, int depth);
uint64_t perft_divide(const Position* pos, int depth);
