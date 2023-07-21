//local
#include "../log/logger.h"
#include "../include/error.h"
#include "../include/ld_types.h"
#include "../include/message.h"
#include "../include/parser.h"
#include "../include/pgld.h"

//postgres-based
#include <fmgr.h>
#include "postmaster/bgworker.h"
#include "storage/shmem.h"

//posix
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

/* --- Local global vars --- */

routine_function_t routine = NULL;

static cluster_t* hacluster = NULL;
static node_t* node = NULL;
static message_t* message_buffer;

static unsigned int quorum_size;

static struct timeval timeval_temp;

/* --- SQL used function to display info ---  */

PG_FUNCTION_INFO_V1(get_node_id);
PG_FUNCTION_INFO_V1(get_cluster_config);
PG_FUNCTION_INFO_V1(get_leader_id);

/* --- Income message handlers for each state --- */

static pl_error_t handle_message_follower(void);
static pl_error_t handle_message_candidate(unsigned int*);
static pl_error_t handle_message_leader(void);

/* --- Local auxiliary functions */

static pl_error_t network_init(void);
static void main_cycle(void);

#define assign_new_random_timeout(_sockfd, _min, _max)                                                                          \
                            timeval_temp.tv_sec = 1;                                                                            \
                            timeval_temp.tv_usec = _min + rand()%(_max-_min);                                                   \
                            if( setsockopt(_sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeval_temp, sizeof(struct timeval)) == -1){     \
                                leadlog("INFO", "Error while assigning new timeout on socket %d equals %ld usec.",              \
                                 _sockfd, timeval_temp.tv_usec);                                                                \
                                leadlog("INFO", "%s", strerror(errno));                                                         \
                            } else {                                                                                            \
                                leadlog("INFO", "New timeout = %ld usec. assigned.", timeval_temp.tv_usec)                      \
                            }                                                                                                   \

#define current_node_wrap(_message)  _message.sender_id = node->node_id;                     \
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
    PG_RETURN_INT32(-1);
}


PGDLLEXPORT void
node_routine(Datum datum)
{
    BackgroundWorkerUnblockSignals();

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
    }

    quorum_size = ceil((float)(hacluster->n_nodes)/2.0);
    srand(time(NULL)); // rand() will be used to assign random timeout

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
                leadlog("INFO", "As follower - timeout");
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
    unsigned int electorate_size = 1;

    message_t elect_req;
    elect_req.type = ElectionRequest;
    ++(node->current_node_term);
    current_node_wrap(elect_req);

    //send everyone except for itself election request
    for(int i = 0; i < hacluster->n_nodes; ++i){
        if(i != node->node_id){
            leadlog("INFO", "As candidate - sending message to %s:%d. (%ld term)", 
                inet_ntoa(hacluster->node_addresses[i].sin_addr), hacluster->node_addresses[i].sin_port,
                node->current_node_term);

            if(write(node->outsock[i], &elect_req, sizeof(message_t)) == -1){
                leadlog("ERROR", "As candidate - sending message to %s:%d error (%s)", 
                inet_ntoa(hacluster->node_addresses[i].sin_addr), hacluster->node_addresses[i].sin_port,
                strerror(errno));
            }
        }
    }

    //wait for responces
    while(1){
        int error;
        assign_new_random_timeout(node->insock, node->min_timeout, node->max_timeout);

        if((error = read(node->insock, message_buffer, sizeof(message_t))) == -1){
            if(errno == EWOULDBLOCK){ //timeout
                leadlog("INFO", "As candidate - timeout."); 
                GOTO_candidate(node);
            } else {
                POSIX_THROW(SOCKET_READ_ERROR);
            }
        } else {
            node_state_t prev_state = node->state;

            SAFE(handle_message_candidate(&electorate_size));

            // if handle_message_candidate() changes node state
            // then make return from that state
            if(prev_state != node->state){
                RETURN_SUCCESS();
            }
        }
    }
}

pl_error_t
leader_routine(void)
{   
    struct timeval heartbeat_timeout = { .tv_sec = 0, .tv_usec = node->heartbeat_timeout };

    message_t heartbeat_message;
    heartbeat_message.type = Heartbeat;
    current_node_wrap(heartbeat_message);

    if( setsockopt(node->insock, SOL_SOCKET, SO_RCVTIMEO, &heartbeat_timeout, sizeof(struct timeval)) != 0) {
        leadlog("ERROR", "Error while assigning new timeout on socket %d equals %ld sec. %ld usec. (%s)",
                node->insock, heartbeat_timeout.tv_usec, heartbeat_timeout.tv_usec, strerror(errno));
    }

    while(1){
        if( read(node->insock, message_buffer, sizeof(message_t)) == -1 ){
            if( errno == EWOULDBLOCK ){ //timeout
                leadlog("INFO", "As leader - timeout, make heartbeat %ld.", heartbeat_timeout.tv_usec); 
                for(int i = 0; i < hacluster->n_nodes; ++i){ // send everyone heartbeat (except for itself)
                    if(i != node->node_id){
                        if( write(node->outsock[i], &heartbeat_message, sizeof(message_t)) == -1){
                            leadlog("ERROR", "Error while sending heartbeat from %s:%d to %s:%d. (%s)",
                                    inet_ntoa(hacluster->node_addresses[node->node_id].sin_addr), 
                                    hacluster->node_addresses[node->node_id].sin_port,
                                    inet_ntoa(hacluster->node_addresses[i].sin_addr), 
                                    hacluster->node_addresses[i].sin_port,
                                    strerror(errno));
                        }
                    }
                }
            } else {
                POSIX_THROW(SOCKET_WRITE_ERROR);
            }
        } else {
            node_state_t prev_state = node->state;

                SAFE(handle_message_leader());

                // if handle_message_leader() changes node state
                // then make return from that state
                if(prev_state != node->state){
                    RETURN_SUCCESS();
                }
        }
    }

}

pl_error_t
handle_message_follower()
{
    message_type_t type = message_buffer->type;
    int s_id = message_buffer->sender_id;
    int s_term = message_buffer->sender_term;

    if(s_term < node->current_node_term){
        RETURN_SUCCESS();
    }

    switch (type)
    {

    case Heartbeat:
        leadlog("INFO", "As follower - got message type of Hearbeat from %d with %d term", s_id, s_term);
        node->leader_id = s_id;
        if(s_term > node->current_node_term){
            node->current_node_term = s_term;
        }

        break;

    case ElectionRequest:
        leadlog("INFO", "As follower - got message type of ElectionRequest from %d with %d term", s_id, s_term);
        if(s_term > node->current_node_term){
            message_t response;

            node->current_node_term = s_term;
            
            current_node_wrap(response);
            response.type = ElectionResponse;

            if(write(node->outsock[s_id], &response, sizeof(message_t)) == -1){
                POSIX_THROW(SOCKET_WRITE_ERROR);
            }
        }

        break;

    case ElectionResponse:
        leadlog("WRONG", "As follower - got message type of ElectionResponse which doesn't handling");

        break;
    }

    RETURN_SUCCESS();
}

pl_error_t
handle_message_candidate(unsigned int* electorate_size)
{
    message_type_t type = message_buffer->type;
    int s_term = message_buffer->sender_term;

    if(s_term < node->current_node_term){
        leadlog("INFO", "As candidate - %d term of incoming message less that %ld", s_term, node->current_node_term); 
        RETURN_SUCCESS();
    }

    switch (type)
    {
    case ElectionRequest:
        leadlog("INFO", "As candidate - got ElectionRequest with %d term (current %ld)", s_term, node->current_node_term); 
        if(s_term > node->current_node_term){
            node->current_node_term = s_term;
            GOTO_follower(node);
        }
        break;
    case Heartbeat:
        leadlog("INFO", "As candidate - got Heatbeat with %d term (current %ld)", s_term, node->current_node_term); 
        if(s_term >= node->current_node_term){
            node->current_node_term = s_term;
            GOTO_follower(node);
        }
        break;
    case ElectionResponse:
        ++(*electorate_size);
        leadlog("INFO", "As candidate - got ElectionResponse (current electorate_size %d of %d)", *electorate_size, quorum_size); 
        if(*electorate_size >= quorum_size){
            GOTO_leader(node);
        }
        break;
    }

    RETURN_SUCCESS();
}

pl_error_t
handle_message_leader()
{
    message_type_t type = message_buffer->type;
    int s_term = message_buffer->sender_term;

    if(s_term < node->current_node_term){
        RETURN_SUCCESS();
    }

    switch (type)
    {
    case ElectionRequest:
        leadlog("INFO", "As leader - got ElectionRequest (sender term %d, current term %ld)", s_term, node->current_node_term); 
        if(s_term > node->current_node_term){
            node->current_node_term = s_term;
            GOTO_follower(node);
        }
        break;
    case Heartbeat:
        leadlog("INFO", "As leader - got Heartbeat (sender term %d, current term %ld)", s_term, node->current_node_term);  
        if(s_term >= node->current_node_term){
            node->current_node_term = s_term;
            GOTO_follower(node);
        }
        break;
    default:
        THROW(WRONG_MESSAGE_DESTINATION_ERROR, 
             "node in leader state can not handle %d type of message", type);
        break;
    }

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
    
    if( setsockopt(insock, SOL_SOCKET, SO_SNDBUF, &timeval_temp, sizeof(struct timeval)) == -1){     \
        leadlog("ERROR", "Error while opt buf put (%s)", strerror(errno));
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
