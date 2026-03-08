#ifndef BITBOARD_H
#define BITBOARD_H

#include <stdint.h>
#include <assert.h>

#ifdef _MSC_VER
#include <intrin.h>
#endif

typedef uint64_t Bitboard;

static inline int popcount64(Bitboard bb)
{
#if defined(_MSC_VER)
    return (int)__popcnt64(bb);
#else
    return __builtin_popcountll(bb);
#endif
}

static inline int lsb(Bitboard bb)
{
    assert(bb);

#if defined(_MSC_VER)
    unsigned long idx;
    _BitScanForward64(&idx, bb);
    return (int)idx;
#else
    return __builtin_ctzll(bb);
#endif
}

static inline int pop_lsb(Bitboard* bb)
{
    assert(*bb);
    int sq = lsb(*bb);
    *bb &= *bb - 1;
    return sq;
}

#endif
