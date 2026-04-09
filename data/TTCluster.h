#ifndef TTCLUSTER_H
#define TTCLUSTER_H

#include "TTEntry.h"

#define CLUSTER_SIZE 4

typedef struct {
    TTEntry entries[CLUSTER_SIZE]; 
} TTCluster;

static inline TTEntry* ttcluster_get(TTCluster* c, int idx) {
    return &c->entries[idx];
}
static inline void ttcluster_set(TTCluster* c, int idx, TTEntry e) {
    c->entries[idx] = e;
}
static inline int ttcluster_isEmpty(TTCluster* c, int idx) {
    return ttentry_depth(&c->entries[idx]) == 0;
}

#endif // TTCLUSTER_H