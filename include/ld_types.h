#ifndef HA_TYPES_H
#define HA_TYPES_H

#include <netinet/in.h>
#include <sys/time.h>
#include <time.h>

typedef unsigned long term_t;
typedef struct sockaddr_in socket_node_info_t;
typedef int socket_fd_t;
typedef int32_t node_id_t;

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
 * This struct contains fields that can be accessed via shared memory
 * from other postgres proccesses.
 */
struct shared_info_node
{
    // node id is unique in range 0, ... , n_nodes-1
    node_id_t node_id;
    node_id_t leader_id;
    term_t current_node_term;
};

/*
 * Each node condition.
 */
typedef struct node
{
    // udp-socket for incoming messages
    socket_fd_t insock; 
    // udp-socket for outcomig messages to other nodes by it id: outsock[id]
    socket_fd_t* outsock; 

    //current state of the node
    node_state_t state;
    //current states of other nodes according to the opinion of this one
    //node_state_t* all_states;

    struct shared_info_node* shared;

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
    int32_t n_nodes;
} cluster_t;

#endif /* HA_TYPES_H */
