#include "../include/node.h"

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
