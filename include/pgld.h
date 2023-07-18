#ifndef PGHA_H
#define PGHA_H

#include "postgres.h"
#include "error.h"

/* --- Each node condiotion has a function describing it --- */
typedef pl_error_t (*routine_function_t)(void);

/* --- Initial pg_leader node fuction --- */
PGDLLEXPORT void node_routine(Datum);

#define GOTO_leader

#define GOTO_follower

#define GOTO_candidate

#endif  /* PGHA_H */