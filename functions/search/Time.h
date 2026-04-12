#ifndef TIME_H
#define TIME_H

#include <stdint.h>
#include <stdbool.h>

extern volatile bool search_aborted;
extern uint64_t search_end_time;
extern uint64_t search_nodes;

uint64_t get_time_ms(void);
void check_time(void);

#endif
