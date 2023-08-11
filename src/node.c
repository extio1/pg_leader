#include "../include/node.h"
#include "../include/error.h"

#include <postgres.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static const int linear_coefficient = 1000;

void 
assign_min_timeout(const int val)
{
    node->min_timeout = val * linear_coefficient;
}

void 
assign_max_timeout(const int val)
{
    node->max_timeout = val * linear_coefficient;
}

void 
assign_heartbeat_timeout(const int val)
{
    node->heartbeat_timeout = val * linear_coefficient;
}

pl_error_t
assign_new_leader(const int val)
{    
    node->shared->leader_id = val;

    RETURN_SUCCESS();
}
