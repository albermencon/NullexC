#include <stdlib.h>
#include "TranspositionTable.h"

#define CLUSTER_REPLACEMENT_AGE_MULT 8

void tt_init(TranspositionTable* tt, int sizeMB) {
    int bytesPerCluster = CLUSTER_SIZE * sizeof(uint64_t);
    int totalClusters   = (sizeMB * 1024 * 1024) / bytesPerCluster;

    // round down to power of two
    int clustersPow2 = 1;
    while (clustersPow2 * 2 <= totalClusters) clustersPow2 *= 2;

    tt->numCluster  = clustersPow2;
    tt->clusterMask = clustersPow2 - 1;

    int clusterBits = 0;
    while ((1 << clusterBits) < clustersPow2) clusterBits++;
    tt->shiftHigh = 64 - clusterBits;

    tt->table = (TTCluster*) calloc(clustersPow2, sizeof(TTCluster));

    tt->generation = 0;
}

void tt_newSearch(TranspositionTable* tt) {
    tt->generation = (tt->generation + 1) & ((1 << GENERATION_BITS) - 1);
}

int tt_clusterIndex(const TranspositionTable* tt, uint64_t hash) {
    return (int)((hash >> tt->shiftHigh) & tt->clusterMask);
}

void tt_free(TranspositionTable* tt) {
    free(tt->table);
    tt->table = NULL;
}

// Probe a cluster by hash. Returns a pointer to the matching entry, or NULL if not found
TTEntry* tt_probe(TranspositionTable* tt, uint64_t hash) {
    int clusterIdx = tt_clusterIndex(tt, hash);
    TTCluster* cluster = &tt->table[clusterIdx];
    uint16_t key16 = (uint16_t)(hash & KEY_MASK);

    for (int i = 0; i < CLUSTER_SIZE; i++) {
        TTEntry* e = ttcluster_get(cluster, i);
        if (ttentry_key16(e) == key16) {
            return e;
        }
    }
    return NULL;
}

// Store an entry in a cluster, replacing the "worst" entry if necessary
void tt_store(TranspositionTable* tt, uint64_t hash,
              int depth, int nodeType, int16_t eval, int moveEncoded) {

    int clusterIdx = tt_clusterIndex(tt, hash);
    TTCluster* cluster = &tt->table[clusterIdx];
    uint16_t key16 = (uint16_t)(hash & KEY_MASK);
    int generation = tt->generation;

    int replaceIdx = 0;
    int worstScore = 0x7FFFFFFF;

    for (int i = 0; i < CLUSTER_SIZE; i++) {
        TTEntry* e = ttcluster_get(cluster, i);

        // If the entry matches, update it only if depth is greater or equal
        if (ttentry_key16(e) == key16) {
            if (depth >= ttentry_depth(e)) {
                ttcluster_set(cluster, i,
                              ttentry_make(key16, depth, generation, nodeType, eval, moveEncoded));
            }
            return;
        }

        // Replacement policy: depth - 8 * age
        int age   = (generation - ttentry_generation(e)) & ((1 << GENERATION_BITS) - 1);
        int score = ttentry_depth(e) - (CLUSTER_REPLACEMENT_AGE_MULT * age);

        if (score < worstScore) {
            worstScore = score;
            replaceIdx = i;
        }
    }

    // Replace the "worst" entry
    ttcluster_set(cluster, replaceIdx,
                  ttentry_make(key16, depth, generation, nodeType, eval, moveEncoded));
}