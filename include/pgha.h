#ifndef RAFT_H
#define RAFT_H

#include "postgres.h"

/*
 *
 */
PGDLLEXPORT node_init(Datum);

void follower_routine();
void candidate_routine();
void leader_routine();

#endif  /* RAFT_H */