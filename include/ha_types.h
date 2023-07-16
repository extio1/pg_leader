#ifndef HA_TYPES_H
#define HA_TYPES_H

#include <netinet/in.h>
/*
 * Monotonic term counter
 */
typedef unsigned long term_t;

typedef struct sockaddr_in socket_node_info_t;

/*
 * Possible states.
 */
typedef enum node_state
{
    Follower = 0,
    Candidate,
    Leader
} node_state_t;

/*
 * Each node condition.
 */
typedef struct node
{
    int node_id;
    node_state_t state;
    term_t current_node_term;
} node_t;


/*
 * Describes parsed structure of the cluster.
 */
typedef struct cluster
{
    socket_node_info_t* node_addresses;
    size_t n_nodes;
} cluster_t;

#endif /* HA_TYPES_H */
