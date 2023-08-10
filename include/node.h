#ifndef LD_NODE_H
#define LD_NODE_H

#include "ld_types.h"
#include "error.h"

extern node_t* node;

/*
 * Encapsulation of assigning milliseconds. (ms will be probably transormed in another units)
 * aka setters
 */

void assign_min_timeout(const int);
void assign_max_timeout(const int);
void assign_heartbeat_timeout(const int);
pl_error_t assign_new_leader(const int);

#endif /* LD_NODE_H */