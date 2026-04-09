// TranspositionTable.h
#ifndef TRANSPOSITION_TABLE_H
#define TRANSPOSITION_TABLE_H

#include <stdint.h>
#include "TTCluster.h"

typedef struct {
    TTCluster* table;     // array of clusters
    uint64_t   numCluster;
    uint64_t   clusterMask;
    uint64_t   shiftHigh;
    int        generation; // 6-bit cyclic
} TranspositionTable;

// Function prototypes
void tt_init(TranspositionTable* tt, int sizeMB);
void tt_newSearch(TranspositionTable* tt);
int  tt_clusterIndex(const TranspositionTable* tt, uint64_t hash);
void tt_free(TranspositionTable* tt);

// Probe/store functions
TTEntry* tt_probe(TranspositionTable* tt, uint64_t hash);
void     tt_store(TranspositionTable* tt, uint64_t hash,
                  int depth, int nodeType, int16_t eval, int moveEncoded);



#endif // TRANSPOSITION_TABLE_H