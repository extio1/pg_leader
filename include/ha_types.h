#ifndef HA_TYPES_H
#define HA_TYPES_H

#include "hashtable.h"
/*
 * Monotonic term counter
 */
typedef unsigned long term_t;

/*
 * Possible states
 */
typedef enum node_state
{
    Follower = 0,
    Candidate,
    Leader
} node_state_t;

/*
 * Each node condition
 */
typedef struct node
{
    int node_id;
    node_state_t state;
    term_t current_node_term;
} node_t;

typedef struct cluster
{
    hash_table* node_addresses;
    size_t n_nodes;
} cluster_t;

#endif /* HA_TYPES_H */
