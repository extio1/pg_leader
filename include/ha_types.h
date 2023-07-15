
/*
 * Monotonic term counter
 */
typedef unsigned long term_t;

/*
 * Possible states
 */
typedef enum
{
    Follower = 0,
    Candidate,
    Leader
} node_state_t;

/*
 * Each node condition
 */
typedef struct
{
    node_state_t state;
    term_t current_node_term;
} raft_node_t;
