//local
#include "../log/logger.h"
#include "../include/error.h"
#include "../include/ld_types.h"
#include "../include/message.h"
#include "../include/parser.h"
#include "../include/pgld.h"

//postgres-based
#include <fmgr.h>

//posix
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <time.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

routine_function_t routine = NULL;

static cluster_t* hacluster = NULL;
static node_t* node = NULL;
static message_t* message_buffer;

/* --- SQL used function to display info ---  */

PG_FUNCTION_INFO_V1(get_node_id);
PG_FUNCTION_INFO_V1(get_cluster_config);
PG_FUNCTION_INFO_V1(get_leader_id);

/* --- Income message handlers for each state --- */

static pl_error_t handle_message_follower();
static pl_error_t handle_message_candidate();
static pl_error_t handle_message_leader();

static pl_error_t network_init(void);
static void main_cycle(void);

static struct timeval timeval_temp;
#define assign_new_random_timeout(_sockfd, _min, _max)                                                                          \
                            timeval_temp.tv_sec = 0;                                                                            \
                            timeval_temp.tv_usec = _min + rand()%(_max);                                                        \
                            if( setsockopt(_sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeval_temp, sizeof(struct timeval)) == -1){     \
                                leadlog("INFO", "Error while assigning new timeout on socket %d equals %ld usec.",              \
                                 _sockfd, timeval_temp.tv_usec);                                                                \
                            }                                                                                                   \

#define current_state_wrap(_message) _message.sender_id = node->node_id;                     \
                                     _message.sender_state = node->state;                    \
                                     _message.sender_term = node->current_node_term;         \

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
Datum
get_leader_id(PG_FUNCTION_ARGS)
{

    PG_RETURN_INT32(node->leader_id);
}


PGDLLEXPORT void
node_routine(Datum datum)
{
    hacluster = malloc(sizeof(cluster_t));    
    node = malloc(sizeof(node_t));
    message_buffer = malloc(sizeof(message_buffer));

    if(node == NULL || hacluster == NULL || message_buffer == NULL){
        elog(FATAL, "couldn't malloc for node or cluster or message_buffer structures");
    }  
    node->current_node_term = 1;
    node->state = Follower;

    STRICT(parse_cluster_config("pg_leader_config/cluster.config", hacluster));
    STRICT(parse_node_config("pg_leader_config/node.config", node));

    node->outsock = malloc(sizeof(socket_fd_t)*hacluster->n_nodes);
    if(node->outsock == NULL){
        elog(FATAL, "couldn't malloc for node->outsock or node->all_states");
    }

    STRICT(parse_timeout_config("pg_leader_config/timeout.config", node));

    STRICT(network_init());

    if(init_lead_logger("pg_leader.log") == NULL){
        elog(LOG, "pg_leader logger has not been inited");
        perror(errno);
    }

    srand(time(NULL));

    routine = follower_routine;

    leadlog("INFO", "Launching in the follower state");
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
    int red;
    
    assign_new_random_timeout(node->insock, node->min_timeout, node->max_timeout);

    while(1){
        red = read(node->insock, message_buffer, sizeof(message_t));
        
        if(red < 0)
        {
            if (errno == EWOULDBLOCK){ //timeout
                GOTO_candidate(node);
            }
            else{
                POSIX_THROW(SOCKET_READ_ERROR);
            }
        } 
        else 
        {
            SAFE(handle_message_follower());
            assign_new_random_timeout(node->insock, node->min_timeout, node->max_timeout);
        }
    }

}

pl_error_t
candidate_routine(void)
{

    while(1){
        leadlog("INFO", "CANDIDATE");
        sleep(3);
    }

    GOTO_leader(node);
}

pl_error_t
leader_routine(void)
{   


    GOTO_follower(node);
}

pl_error_t
handle_message_follower()
{
    if(message_buffer->sender_term < node->current_node_term){
        RETURN_SUCCESS();
    }

    switch (message_buffer->type)
    {
    case Heartbeat:
        node->leader_id = message_buffer->sender_id;
        if(message_buffer->sender_term > node->current_node_term){
            node->current_node_term = message_buffer->sender_term;
        }

        break;
    case ElectionRequest:
        if(message_buffer->sender_term > node->current_node_term){
            message_t response;

            node->current_node_term = message_buffer->sender_term;
            
            current_state_wrap(response);
            response.type = ElectionResponse;

            if(write(node->outsock[message_buffer->sender_id], &response, sizeof(message_t)) == -1){
                POSIX_THROW(SOCKET_WRITE_ERROR);
            }
        }
        break;
    default:
        THROW(WRONG_MESSAGE_DESTINATION_ERROR, 
             "node in follower state can not handle %d type of message",
             message_buffer->type);
        break;
    }

    RETURN_SUCCESS();
}

pl_error_t
handle_message_candidate()
{
    RETURN_SUCCESS();
}

pl_error_t
handle_message_leader()
{
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
