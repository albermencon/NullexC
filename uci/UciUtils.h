#ifndef UCI_UTILS_H
#define UCI_UTILS_H

#include "core/types.h"

Square uci_to_sq(const char* s);

void print_uci_move(Move move);

#endif // UCI_UTILS_H
