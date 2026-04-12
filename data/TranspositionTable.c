#include <stdlib.h>
#include <string.h>
#include "TranspositionTable.h"

#define CLUSTER_REPLACEMENT_AGE_MULT 4

void tt_init(TranspositionTable* tt, int sizeMB) {
    // Each cluster is 4 entries
    size_t bytesPerCluster = sizeof(TTCluster);
    size_t totalBytes = (size_t)sizeMB * 1024 * 1024;
    uint64_t clustersCount = totalBytes / bytesPerCluster;

    // Round down to power of two
    uint64_t clustersPow2 = 1;
    while (clustersPow2 * 2 <= clustersCount) clustersPow2 *= 2;

    tt->numCluster  = clustersPow2;
    tt->clusterMask = clustersPow2 - 1;
    
    // Calculate shift for the high-bit index mapping
    int clusterBits = 0;
    while (((uint64_t)1 << clusterBits) < clustersPow2) clusterBits++;
    tt->shiftHigh = 64 - clusterBits;

    tt->table = (TTCluster*)calloc(tt->numCluster, sizeof(TTCluster));
    tt->generation = 0;
}

void tt_newSearch(TranspositionTable* tt) {
    tt->generation = (tt->generation + 1) & 0xFF;
}

int tt_clusterIndex(const TranspositionTable* tt, uint64_t hash) {
    // Use high bits for indexing to leave low bits for the key check
    return (int)((hash >> tt->shiftHigh) & tt->clusterMask);
}

void tt_free(TranspositionTable* tt) {
    if (tt->table) free(tt->table);
    tt->table = NULL;
}

// Probe a cluster by hash. Returns a pointer to the matching entry, or NULL if not found
TTEntry* tt_probe(TranspositionTable* tt, uint64_t hash) {
    int clusterIdx = tt_clusterIndex(tt, hash);
    TTCluster* cluster = &tt->table[clusterIdx];

    for (int i = 0; i < CLUSTER_SIZE; i++) {
        if (cluster->entries[i].key == hash) {
            return &cluster->entries[i];
        }
    }
    return NULL;
}

void tt_store(TranspositionTable* tt, uint64_t hash,
              int depth, int nodeType, int16_t eval, int moveEncoded) {

    TTCluster* cluster = &tt->table[tt_clusterIndex(tt, hash)];

    int replaceIdx = 0;
    int worstScore = 0x7FFFFFFF;

    for (int i = 0; i < CLUSTER_SIZE; i++) {
        TTEntry* e = &cluster->entries[i];

        // If exact hash match, only overwrite if new search is deeper
        if (e->key == hash) {
            if (depth >= ttentry_depth(e)) {
                *e = ttentry_make(hash, depth, tt->generation, nodeType, eval, moveEncoded);
            }
            return;
        }

        // Replacement logic: favor keeping deeper and younger entries
        // Age is calculated with circular wrapping
        int age = (tt->generation - ttentry_gen(e)) & 0xFF;
        int score = ttentry_depth(e) - (CLUSTER_REPLACEMENT_AGE_MULT * age);

        if (score < worstScore) {
            worstScore = score;
            replaceIdx = i;
        }
    }

    cluster->entries[replaceIdx] = ttentry_make(hash, depth, tt->generation, nodeType, eval, moveEncoded);
}
