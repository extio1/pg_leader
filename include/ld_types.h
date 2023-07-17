#ifndef HA_TYPES_H
#define HA_TYPES_H

#include <netinet/in.h>
/*
 * Monotonic term counter
 */
typedef unsigned long term_t;

typedef struct sockaddr_in socket_node_info_t;
typedef int socket_fd_t;

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
    // tcp-socket fds using to communicate with another nodes
    socket_fd_t* sock_node;
    // node id is unique in range 0, ... , n_nodes-1
    uint32_t node_id;
    node_state_t state;
    term_t current_node_term;
    unsigned int min_timeout;
    unsigned int max_timeout;
    unsigned int heartbeat_timeout;
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