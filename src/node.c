#include "../include/node.h"
#include "../include/visualization.h"
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
    char message[20];

    node->shared->leader_id = val;

    if( (cluster_info_pipe_fd = open(pipe_cluster_state_path, O_WRONLY)) < 0){
        POSIX_THROW(OPEN_ERROR);
    }

    sprintf(message, "leader %d %d\n", node->shared->node_id, node->shared->leader_id);

    static int size = 6 + 2 + 2 + 2;
    if( write(cluster_info_pipe_fd, message, size) < 0 ){
        POSIX_THROW(WRITE_ERROR);
    }

    close(cluster_info_pipe_fd);
    RETURN_SUCCESS();
}
