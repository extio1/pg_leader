#ifndef HA_TYPES_H
#define HA_TYPES_H

#include <netinet/in.h>
/*
 * Monotonic term counter
 */
typedef unsigned long term_t;

typedef struct sockaddr_in socket_node_info_t;
typedef int socket_fd_t;
typedef uint32_t node_id_t;

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
    // udp-socket for incoming messages
    socket_fd_t insock; 
    // udp-socket for outcomig messages to other nodes by it id: outsock[id]
    socket_fd_t* outsock; 
    // node id is unique in range 0, ... , n_nodes-1
    node_id_t node_id;

    //current state of the node
    node_state_t state;
    //current states of other nodes according to the opinion of this one
    //node_state_t* all_states;
    node_id_t leader_id;

    term_t current_node_term;

    //time in microseconds
    unsigned int min_timeout;
    unsigned int max_timeout;
    struct timeval current_timeout;
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
