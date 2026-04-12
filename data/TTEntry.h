#ifndef TT_ENTRY
#define TT_ENTRY

#include <stdint.h>

// 50 bytes
#define DEPTH_BITS       8
#define GENERATION_BITS  8
#define TYPE_BITS        2
#define EVAL_BITS        16
#define MOVE_BITS        16

#define DEPTH_SHIFT      0
#define GEN_SHIFT        8
#define TYPE_SHIFT       16
#define EVAL_SHIFT       18
#define MOVE_SHIFT       34

// Node type constants
#define TYPE_EXACT        0b00
#define TYPE_LOWER_BOUND  0b01
#define TYPE_UPPER_BOUND  0b11

typedef struct {
    uint64_t key;   // Full 64-bit Zobrist Hash
    uint64_t data;  // Packed metadata
} TTEntry;

static inline TTEntry ttentry_make(uint64_t hash, int depth, int gen, int type, int16_t eval, int move) {
    TTEntry e;
    e.key = hash;
    e.data = ((uint64_t)(uint8_t)depth << DEPTH_SHIFT) |
             ((uint64_t)(uint8_t)gen   << GEN_SHIFT)   |
             ((uint64_t)(uint8_t)type  << TYPE_SHIFT)  |
             ((uint64_t)(uint16_t)eval << EVAL_SHIFT)  |
             ((uint64_t)(uint16_t)move << MOVE_SHIFT);
    return e;
}

static inline int16_t ttentry_eval(const TTEntry* e) { return (int16_t)(e->data >> EVAL_SHIFT); }
static inline int     ttentry_move(const TTEntry* e) { return (int)(uint16_t)(e->data >> MOVE_SHIFT); }
static inline int     ttentry_depth(const TTEntry* e) { return (int)(uint8_t)(e->data >> DEPTH_SHIFT); }
static inline int     ttentry_gen(const TTEntry* e)   { return (int)(uint8_t)(e->data >> GEN_SHIFT); }
static inline int     ttentry_type(const TTEntry* e)  { return (int)((e->data >> TYPE_SHIFT) & 0x3); }

#endif // TT_ENTRY
