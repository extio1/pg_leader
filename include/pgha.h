#ifndef PGHA_H
#define PGHA_H

#include "postgres.h"
#include "error.h"

/*
 *
 */
PGDLLEXPORT error_code_t node_init(Datum);

error_code_t follower_routine(void);
error_code_t candidate_routine(void);
error_code_t leader_routine(void);

#endif  /* PGHA_H */