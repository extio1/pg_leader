#include "../log/logger.h"
#include "../include/error.h"
#include "../include/pgld.h"
#include "../include/ld_types.h"
#include "../include/parser.h"

#include "postmaster/bgworker.h"

#include <arpa/inet.h>
#include <stdlib.h>
#include <math.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <fcntl.h>

cluster_t* hacluster = NULL;
node_t* node = NULL;
unsigned int quorum_size = 0;
routine_function_t routine = NULL;
message_t* message_buffer = NULL;

static pl_error_t network_init(void);

PGDLLEXPORT void
node_routine(Datum datum)
{
    BackgroundWorkerUnblockSignals();

    if(shared_info_node == NULL){
        elog(FATAL, "error while initializing shared memory");
    }

    hacluster = malloc(sizeof(cluster_t));    
    node = malloc(sizeof(node_t));
    message_buffer = malloc(sizeof(message_buffer));

    if(node == NULL || hacluster == NULL || message_buffer == NULL){
        elog(FATAL, "couldn't malloc for node or cluster or message_buffer structures");
    }  

    node->shared = shared_info_node;
    node->state = Follower;

    STRICT(parse_cluster_config("pg_leader_config/cluster.config", hacluster));
    STRICT(parse_node_config("pg_leader_config/node.config", node));

    node->shared->node_id = node->node_id;
    node->outsock = malloc(sizeof(socket_fd_t)*hacluster->n_nodes);
    if(node->outsock == NULL){
        elog(FATAL, "couldn't malloc for node->outsock or node->all_states");
    }

    STRICT(parse_timeout_config("pg_leader_config/timeout.config", node));

    STRICT(network_init());

    if(init_lead_logger("pg_leader.log") == NULL){
        elog(LOG, "pg_leader logger has not been inited");
    }

    quorum_size = ceil((float)(hacluster->n_nodes)/2.0);
    srand(time(NULL)); // rand() will be used to assign random timeout

    routine = follower_routine;

    leadlog("INFO", "Launching in the follower state");
    main_cycle();
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

        if( fcntl(outsock, F_SETFL, O_NONBLOCK) == -1){
            POSIX_THROW(SOCKET_OPT_SET_ERROR);
        }

        if( connect(outsock, (struct sockaddr*)&outaddr, sizeof(struct sockaddr_in)) != 0 ){
            THROW(SOCKET_CONNECT_ERROR, "Error while connecting #%d node to %s:%d",
                  node->node_id, inet_ntoa(outaddr.sin_addr), outaddr.sin_port);
        }

        node->outsock[i] = outsock;
    }

    RETURN_SUCCESS();
}