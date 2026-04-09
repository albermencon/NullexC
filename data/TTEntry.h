// TTEntryPacked.h
#ifndef TT_ENTRY
#define TT_ENTRY

#include <stdint.h>

// 10 bytes
#define KEY_BITS        16
#define DEPTH_BITS       8
#define GENERATION_BITS  6 // 5
#define PV_NODE_BITS     1
#define TYPE_BITS        2
#define EVAL_BITS       16
#define MOVE_BITS       16
#define VALUE_BITS      16

#define KEY_MASK        ((1ULL << KEY_BITS) - 1)
#define DEPTH_MASK      ((1ULL << DEPTH_BITS) - 1)
#define GENERATION_MASK ((1ULL << GENERATION_BITS) - 1)
#define TYPE_MASK       ((1ULL << TYPE_BITS) - 1)
#define EVAL_MASK       ((1ULL << EVAL_BITS) - 1)
#define MOVE_MASK       ((1ULL << MOVE_BITS) - 1)

#define DEPTH_SHIFT      KEY_BITS
#define GENERATION_SHIFT (DEPTH_SHIFT + DEPTH_BITS)
#define TYPE_SHIFT       (GENERATION_SHIFT + GENERATION_BITS)
#define EVAL_SHIFT       (TYPE_SHIFT + TYPE_BITS)
#define MOVE_SHIFT       (EVAL_SHIFT + EVAL_BITS)

// Node type constants
#define TYPE_EXACT        0b00
#define TYPE_LOWER_BOUND  0b01
#define TYPE_UPPER_BOUND  0b11

typedef struct {
    uint64_t packed;
} TTEntry;

static inline TTEntry ttentry_make(
    uint16_t key16, int depth, int generation, int nodeType, int16_t eval, int moveEncoded)
{
    uint64_t k = (uint64_t)key16 & KEY_MASK;
    uint64_t d = (uint64_t)depth & DEPTH_MASK;
    uint64_t g = (uint64_t)generation & GENERATION_MASK;
    uint64_t t = (uint64_t)nodeType & TYPE_MASK;
    uint64_t e = (uint64_t)eval & EVAL_MASK;
    uint64_t m = (uint64_t)moveEncoded & MOVE_MASK;

    TTEntry entry = {
        .packed = (k << 0) |
                  (d << DEPTH_SHIFT) |
                  (g << GENERATION_SHIFT) |
                  (t << TYPE_SHIFT) |
                  (e << EVAL_SHIFT) |
                  (m << MOVE_SHIFT)
    };
    return entry;
}

static inline uint16_t ttentry_key16(const TTEntry* e) {
    return (uint16_t)(e->packed & KEY_MASK);
}
static inline int ttentry_depth(const TTEntry* e) {
    return (int)((e->packed >> DEPTH_SHIFT) & DEPTH_MASK);
}
static inline int ttentry_generation(const TTEntry* e) {
    return (int)((e->packed >> GENERATION_SHIFT) & GENERATION_MASK);
}
static inline int16_t ttentry_eval(const TTEntry* e) {
    return (int16_t)((e->packed >> EVAL_SHIFT) & EVAL_MASK);
}
static inline int ttentry_type(const TTEntry* e) {
    return (int)((e->packed >> TYPE_SHIFT) & TYPE_MASK);
}
static inline int ttentry_move(const TTEntry* e) {
    return (int)((e->packed >> MOVE_SHIFT) & MOVE_MASK);
}

#endif // TT_ENTRY
