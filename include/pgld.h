#ifndef PGHA_H
#define PGHA_H

#include "../log/logger.h"
#include "ld_types.h"
#include "message.h"
#include "error.h"

#include "postgres.h"

/* --- Initial pg_leader node fuction --- */
PGDLLEXPORT void node_init_and_launch(Datum);

/* --- Each node condition has a function describing it --- */
typedef pl_error_t (*routine_function_t)(void);

/* --- Node condition functions ---  */

extern pl_error_t follower_routine(void);
extern pl_error_t candidate_routine(void);
extern pl_error_t leader_routine(void);

/* --- Initialized in init.c --- */
extern cluster_t* hacluster;
extern node_t* node;
extern message_t* message_buffer;
extern unsigned int quorum_size;

// Pointer to fuction describes current state
extern routine_function_t routine;
extern struct shared_info_node* shared_info_node;

// Main cycle launches routine function in routine var. Initialized in routine.c
extern void main_cycle(void);

/* 
 * GOTO_<state> changes state and return from the function
 */

#define GOTO_leader(_node_ptr) _node_ptr->state = Leader;           \
                               routine = &leader_routine;           \
                               leadlog("INFO", "LEADER STATE");     \
                               RETURN_SUCCESS();                    \

#define GOTO_follower(_node_ptr) _node_ptr->state = Follower;        \
                                 routine = &follower_routine;        \
                                 leadlog("INFO", "FOLLOWER STATE");  \
                                 RETURN_SUCCESS();                   \

#define GOTO_candidate(_node_ptr) _node_ptr->state = Candidate;       \
                                  routine = &candidate_routine;       \
                                  leadlog("INFO", "CANDIDATE STATE"); \
                                  RETURN_SUCCESS();                   \


#endif  /* PGHA_H */