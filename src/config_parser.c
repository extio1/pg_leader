#include "../include/parser.h"
#include "../include/node.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include <postgres.h>
#include <utils/guc.h>

pl_error_t 
parse_cluster_config(cluster_t* cluster){
    const int buffer_size = 8192;
    char* buff = malloc(sizeof(buffer_size));
    int32_t n_nodes = 0;

    struct sockaddr_in addr;
    char* strptr;
    int arrptr = 0;

    addr.sin_family = AF_INET;
    bzero(&addr, sizeof(struct sockaddr_in));

    DefineCustomIntVariable(
        "pg_leader.n_nodes",
        "Num of nodes.",
        "Num of nodes.",
        &n_nodes,
        0,
        0, INT32_MAX,
        PGC_SIGHUP,
        0,
        NULL,
        NULL,
        NULL
    );

    cluster->node_addresses = malloc(sizeof(socket_node_info_t)*n_nodes);

    if(cluster->node_addresses == NULL){
        POSIX_THROW(MALLOC_ERROR);
    }

    elog(LOG, "%d nodes in config file.", n_nodes);

    cluster->n_nodes = n_nodes;

    DefineCustomStringVariable(
        "pg_leader.cluster_conf",
        "Cluster configuration: IPs and port for each node",
        "Every node in cluster should be described by IP and port.",
        &buff,
        "",
        PGC_SIGHUP,
        0,
        NULL,
        NULL,
        NULL
    );

    strptr = strtok(buff, " \n:");
    for(int i = 0; strptr != NULL; ++i){

        if(i % 2 == 0){ // ip part
            if(inet_aton(strptr, &addr.sin_addr) == 0){
                POSIX_THROW(WRONG_IPADDR_ERROR);
            }
        } else { // port part
            addr.sin_port = atoi(strptr);
            cluster->node_addresses[arrptr++] = addr;
        }

        strptr = strtok(NULL,  " \n:");
    }

    free(buff);

    RETURN_SUCCESS();
}

pl_error_t 
parse_node_config(node_t* node){
    DefineCustomIntVariable(
        "pg_leader.node_id",
        "The ID of node",
        "Every node in pg_leader has it's own ID, this parameter shows the ID of currently launched node.\
         Max value 300 (limitation because of len of string for IPs)",
        &(node->shared->node_id),   
        -1,
        -1, INT32_MAX,
        PGC_SIGHUP,
        0,
        NULL,
        NULL,
        NULL
    );
    RETURN_SUCCESS();
}

pl_error_t
parse_timeout_config(node_t* node)
{
    int mint, maxt, heart;

    DefineCustomIntVariable(
        "pg_leader.min_timeout",
        "Minimal timeout for waiting for messages.",
        "Minimal timeout for waiting for messages. Used in follower and candidate mode.",
        &mint,   
        0,
        0, 999,
        PGC_SIGHUP,
        0,
        NULL,
        NULL,
        NULL
    );

    DefineCustomIntVariable(
        "pg_leader.max_timeout",
        "Maximal timeout for waiting for messages.",
        "Maximal timeout for waiting for messages. Used in follower and candidate mode.",
        &maxt,   
        0,
        0, 999,
        PGC_SIGHUP,
        0,
        NULL,
        NULL,
        NULL
    );

    DefineCustomIntVariable(
        "pg_leader.heartbeat_timeout",
        "Timeout for sending heartbeat",
        "Every pg_leader.hearbeat_timeout node in leader state will be sending hearbeat messages.",
        &heart,   
        0,
        0, 999,
        PGC_SIGHUP,
        0,
        NULL,
        NULL,
        NULL
    );

    assign_min_timeout(mint);
    assign_max_timeout(maxt);
    assign_heartbeat_timeout(heart);

    RETURN_SUCCESS();
}

pl_error_t
parse_logger_config(char** path, bool* enable){
    DefineCustomBoolVariable(
        "pg_leader.enable_log",
        "Off/on log",
        "Turn on/off log of algorithm",
        enable,
        false,
        PGC_SIGHUP,
        0,
        NULL,
        NULL,
        NULL
    );

    DefineCustomStringVariable(
        "pg_leader.log_file",
        "File where log will be written.",
        "File where log will be written. It's located inside postgres cluster directory.",
        path,
        "pg_leader.log",
        PGC_SIGHUP,
        0,
        NULL,
        NULL,
        NULL
    );

    RETURN_SUCCESS();
}
