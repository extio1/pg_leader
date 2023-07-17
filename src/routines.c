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

/*
 * in SQL used funtion to display info
 */
PG_FUNCTION_INFO_V1(get_node_id);
PG_FUNCTION_INFO_V1(get_cluster_config);

static pl_error_t follower_routine(void);
static pl_error_t candidate_routine(void);
static pl_error_t leader_routine(void);

/*
 * Establishes network connection between nodes all-to-all
 *
 * Node with id in range 0, ... , n_nodes-1 connect to the nodes with id > this.id
 * For instanse (n_nodes=5): 
 *  0 with 1,2,3,4;
 *  1 with 2,3,4;
 *  2 with 3,4
 *  3 with 4
 *  4 with no one
 * 
 *  BAD THING: SERVERS SHOULD BE TURNING ON IN FOLLOWING ORDER: 4 -> 3 -> 2 -> 1 -> 0
 */
static pl_error_t network_init(void);

/*
 * Adds socket fd to node.sock_node.
 * May throws OUT_OF_BOUND_ERROR.
 */
static pl_error_t add_socket(int);


PGDLLEXPORT pl_error_t
node_routine(Datum datum)
{
    hacluster = malloc(sizeof(cluster_t));    
    node = malloc(sizeof(node_t));
    node->current_node_term = 1;

    if(node == NULL || hacluster == NULL){
        THROW(MALLOC_ERROR, "couldn't malloc for node or cluster structures");
    }  

    STRICT(parse_cluster_config("pg_leader_config/cluster.config", hacluster));
    STRICT(parse_node_config("pg_leader_config/node.config", node));
    node->sock_node = malloc(sizeof(socket_fd_t)*hacluster->n_nodes-1);
    STRICT(parse_timeout_config("pg_leader_config/timeout.config", node));

    STRICT(network_init());

    follower_routine();

    elog(LOG, "THANKS FOR WITH LIVE");
    RETURN_SUCCESS();
}


pl_error_t
follower_routine(void)
{
    RETURN_SUCCESS();
}

pl_error_t
candidate_routine(void)
{
    RETURN_SUCCESS();
}

pl_error_t
leader_routine(void)
{   

    RETURN_SUCCESS();
}

pl_error_t 
network_init()
{
    //as server
    if(node->node_id != 0){
        socket_fd_t lstsock, cltsock;
        socklen_t len = sizeof(struct sockaddr_in);

        struct sockaddr_in srvaddr, cltaddr;
        bzero(&srvaddr, sizeof(struct sockaddr_in));
        srvaddr.sin_family = AF_INET;
        srvaddr.sin_addr.s_addr = hacluster->node_addresses[node->node_id].sin_addr.s_addr;
        srvaddr.sin_port = hacluster->node_addresses[node->node_id].sin_port;

        if( (lstsock = socket(AF_INET, SOCK_STREAM, 0)) == -1){
            POSIX_THROW(SOCKET_CREATION_ERROR);
        }

        if(bind(lstsock, (struct sockaddr*)&srvaddr, sizeof(srvaddr)) != 0){
            POSIX_THROW(SOCKET_BIND_ERROR);
        }

        if(listen(lstsock, hacluster->n_nodes-1) != 0){
            POSIX_THROW(SOCKET_LISTEN_ERROR);
        }

        elog(LOG, "#%d node ready to accept %d connections", node->node_id, node->node_id);

        for(int i = 0; i < node->node_id; ++i){
            cltsock = accept(lstsock, (struct sockaddr*) &cltaddr, &len);
            if(cltsock == -1){
                THROW(SOCKET_ACCEPT_ERROR, 
                      "failed to establish the connection between %s:%d (server) and %s:%d (client)",
                      inet_ntoa(srvaddr.sin_addr), srvaddr.sin_port, inet_ntoa(cltaddr.sin_addr), cltaddr.sin_port);
            } else {
                add_socket(cltsock);

                elog(LOG, "%s:%d accepted %s:%d",
                     inet_ntoa(srvaddr.sin_addr), srvaddr.sin_port, inet_ntoa(cltaddr.sin_addr), cltaddr.sin_port);
            }
        }

        close(lstsock);   
    }

    //as client
    if(node->node_id != hacluster->n_nodes-1){
        socket_fd_t sockfd;
        struct sockaddr_in connectaddr;

        for(int i = node->node_id+1; i < hacluster->n_nodes; ++i){
            if( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1){
                POSIX_THROW(SOCKET_CREATION_ERROR);
            }

            bzero(&connectaddr, sizeof(struct sockaddr_in));
            connectaddr.sin_family = AF_INET;
            connectaddr.sin_addr.s_addr = hacluster->node_addresses[i].sin_addr.s_addr;
            connectaddr.sin_port = hacluster->node_addresses[i].sin_port;

            if(connect(sockfd, (struct sockaddr_in*)&connectaddr, sizeof(connectaddr)) != 0){
                THROW(SOCKET_ACCEPT_ERROR, "failed to connect to %s:%d from %s:%d",
                    inet_ntoa(connectaddr.sin_addr), connectaddr.sin_port,
                    inet_ntoa(hacluster->node_addresses[node->node_id].sin_addr),
                    hacluster->node_addresses[node->node_id].sin_port);
            }

            elog(LOG, "%d socket connected to %s:%d",
                 sockfd, inet_ntoa(connectaddr.sin_addr), connectaddr.sin_port);

            add_socket(sockfd);
        }
    }

    RETURN_SUCCESS();
}

pl_error_t
add_socket(int sockfd){
    static int added = 0;
    if(added >= hacluster->n_nodes-1){
        THROW(OUT_OF_BOUND_ERROR, "the try to add one more node of %ld possible", hacluster->n_nodes-1);
    } else {
        node->sock_node[added++] = sockfd;
    }
}

Datum
get_node_id(PG_FUNCTION_ARGS)
{
    PG_RETURN_INT32(node->node_id);
}

Datum
get_cluster_config(PG_FUNCTION_ARGS)
{
    const int record_len = 15+1+5;  //add static?  `
    PG_RETURN_CSTRING(
        "not implemented" 
    );
}