//local
#include "../include/ld_types.h"
#include "../include/parser.h"
#include "../include/pgld.h"

//postgres-based
#include <fmgr.h>

//posix
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>


cluster_t* hacluster = NULL;
node_t* node = NULL;

/* --- SQL used function to display info ---  */

PG_FUNCTION_INFO_V1(get_node_id);
PG_FUNCTION_INFO_V1(get_cluster_config);

/* --- Node condition functions ---  */

static pl_error_t follower_routine(void);
static pl_error_t candidate_routine(void);
static pl_error_t leader_routine(void);

/* --- --- */

static pl_error_t network_init(void);
static void main_cycle(void);

static routine_function_t routine;

Datum
get_node_id(PG_FUNCTION_ARGS)
{
    PG_RETURN_INT32(node->node_id);
}

Datum
get_cluster_config(PG_FUNCTION_ARGS)
{
    PG_RETURN_CSTRING(
        "not implemented" 
    );
}


PGDLLEXPORT void
node_routine(Datum datum)
{
    hacluster = malloc(sizeof(cluster_t));    
    node = malloc(sizeof(node_t));

    if(node == NULL || hacluster == NULL){
        elog(FATAL, "couldn't malloc for node or cluster structures");
    }  
    node->current_node_term = 1;

    STRICT(parse_cluster_config("pg_leader_config/cluster.config", hacluster));
    STRICT(parse_node_config("pg_leader_config/node.config", node));
    node->outsock = malloc(sizeof(socket_fd_t)*hacluster->n_nodes);
    STRICT(parse_timeout_config("pg_leader_config/timeout.config", node));

    STRICT(network_init());

    routine = follower_routine;
    main_cycle();
}

void main_cycle(void){
    while(1){
        SAFE(routine());
    }
}


pl_error_t
follower_routine(void)
{
    routine = candidate_routine;
    RETURN_SUCCESS();
}

pl_error_t
candidate_routine(void)
{
    routine = leader_routine;
    RETURN_SUCCESS();
}

pl_error_t
leader_routine(void)
{   
    routine = follower_routine;
    RETURN_SUCCESS();
}


pl_error_t 
network_init()
{
    struct sockaddr_in inaddr, outaddr;
    socket_fd_t insock, outsock;

    //Income messages socket init
    if((insock = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
        POSIX_THROW(SOCKET_CREATION_ERROR);
    }

    bzero(&inaddr, sizeof(struct sockaddr_in));
    inaddr.sin_family = AF_INET;
    inaddr.sin_addr.s_addr = INADDR_ANY;
    inaddr.sin_port = hacluster->node_addresses[node->node_id].sin_port;

    if( bind(insock, (struct sockaddr*)&inaddr, sizeof(struct sockaddr_in)) != 0 ){
        POSIX_THROW(SOCKET_BIND_ERROR);
    }
    
    node->insock = insock;

    //Outcome messages sockets init
    for(int i = 0; i < hacluster->n_nodes; ++i){
        if(i == node->node_id){
            node->outsock[i] = -1;
            continue;
        }

        if((outsock = socket(AF_INET, SOCK_DGRAM, 0)) == -1){
            POSIX_THROW(SOCKET_CREATION_ERROR);
        }

        bzero(&outaddr, sizeof(struct sockaddr_in));
        outaddr.sin_family = AF_INET;
        outaddr.sin_addr.s_addr = hacluster->node_addresses[i].sin_addr.s_addr;
        outaddr.sin_port = hacluster->node_addresses[i].sin_port;

        if( connect(outsock, (struct sockaddr*)&outaddr, sizeof(struct sockaddr_in)) != 0 ){
            THROW(SOCKET_CONNECT_ERROR, "Error while connecting #%d node to %s:%d",
                  node->node_id, inet_ntoa(outaddr.sin_addr), outaddr.sin_port);
        }

        node->outsock[i] = outsock;
    }

    RETURN_SUCCESS();
}
