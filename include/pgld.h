#ifndef PGHA_H
#define PGHA_H

#include "postgres.h"
#include "error.h"

/* --- Each node condition has a function describing it --- */
typedef pl_error_t (*routine_function_t)(void);

/* --- Node condition functions ---  */

extern pl_error_t follower_routine(void);
extern pl_error_t candidate_routine(void);
extern pl_error_t leader_routine(void);

extern routine_function_t routine;

/* --- Initial pg_leader node fuction --- */
PGDLLEXPORT void node_routine(Datum);

#define GOTO_leader(_node_ptr) _node_ptr->state = Leader;       \
                               routine = &leader_routine;       \
                               elog(LOG, "LEADER STATE");       \
                               RETURN_SUCCESS();                \

#define GOTO_follower(_node_ptr) _node_ptr->state = Follower;    \
                                 routine = &follower_routine;    \
                                 elog(LOG, "FOLLOWER STATE");    \
                                 RETURN_SUCCESS();               \

#define GOTO_candidate(_node_ptr) _node_ptr->state = Candidate;   \
                                  routine = &candidate_routine;   \
                                  elog(LOG, "CANDIDATE STATE");   \
                                  RETURN_SUCCESS();               \


#endif  /* PGHA_H */