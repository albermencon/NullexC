#include "Time.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <time.h>
#endif

volatile bool search_aborted = false;
uint64_t search_end_time = 0;
uint64_t search_nodes = 0;

uint64_t get_time_ms(void) {
#ifdef _WIN32
    LARGE_INTEGER freq, counter;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&counter);
    return (uint64_t)((counter.QuadPart * 1000) / freq.QuadPart);
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)(ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
}

void check_time(void) {
    if (get_time_ms() >= search_end_time) {
        search_aborted = true;
    }
}
