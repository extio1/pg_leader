#include "../log/logger.h"
#include "../include/error.h"
#include "../include/ld_types.h"
#include "../include/message.h"
#include "../include/parser.h"
#include "../include/pgld.h"

#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>


/* --- Income message handlers for each state --- */

static pl_error_t handle_message_follower(void);
static pl_error_t handle_message_candidate(unsigned int*);
static pl_error_t handle_message_leader(void);


static struct timeval timeval_temp;
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

void main_cycle(void){
    while(1){
        SAFE(routine());
    }
}

pl_error_t
follower_routine(void)
{   
    assign_new_random_timeout(node->insock, node->min_timeout, node->max_timeout);
    
    while(1){
        int red = read(node->insock, message_buffer, sizeof(message_t));

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
    unsigned int electorate_size = 1; // counter of nodes voted for us as candidate

    message_t elect_req;
    
    ++(node->shared->current_node_term);

    elect_req.type = ElectionRequest;
    elect_req.sender_id = node->node_id;
    elect_req.sender_state = node->state;
    elect_req.sender_term = node->shared->current_node_term;

    // send everyone except for itself election request
    for(int i = 0; i < hacluster->n_nodes; ++i){
        if(i != node->node_id){
            leadlog("INFO", "As candidate - sending message to %s:%d. (%ld term)", 
                inet_ntoa(hacluster->node_addresses[i].sin_addr), hacluster->node_addresses[i].sin_port,
                node->shared->current_node_term);

            if( write(node->outsock[i], &elect_req, sizeof(message_t)) == -1 ){
                leadlog("ERROR", "As candidate - sending message to %s:%d error (%s)", 
                inet_ntoa(hacluster->node_addresses[i].sin_addr), hacluster->node_addresses[i].sin_port,
                strerror(errno));
            }
        }
    }

    // wait for responces
    while(1){
        int error;
        // timeout while waiting responces
        assign_new_random_timeout(node->insock, node->min_timeout, node->max_timeout);

        if((error = read(node->insock, message_buffer, sizeof(message_t))) == -1){
            if(errno == EWOULDBLOCK){   //timeout
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

    node->shared->leader_id = node->node_id; // this node is a leader now

    heartbeat_message.type = Heartbeat;
    heartbeat_message.sender_id = node->node_id;
    heartbeat_message.sender_state = node->state;
    heartbeat_message.sender_term = node->shared->current_node_term;

    // initialization of timeout for sending heartbeat message each heartbeat_timeout usec.
    if( setsockopt(node->insock, SOL_SOCKET, SO_RCVTIMEO, &heartbeat_timeout, sizeof(struct timeval)) != 0 ) {
        leadlog("ERROR", "Error while assigning new timeout on socket %d equals %ld sec. %ld usec. (%s)",
                node->insock, heartbeat_timeout.tv_usec, heartbeat_timeout.tv_usec, strerror(errno));
    }

    while(1){
        // if no message got while heartbeat_timeout send heartbeat_message,
        // otherwise handle got message
        if( read(node->insock, message_buffer, sizeof(message_t)) == -1 ) {
            if( errno == EWOULDBLOCK ){ //timeout
                leadlog("INFO", "As leader - timeout, make heartbeat %ld.", heartbeat_timeout.tv_usec); 

                for(int i = 0; i < hacluster->n_nodes; ++i){ // send everyone heartbeat (except for itself)
                    if(i == node->node_id){
                        continue;
                    }
                    if( write(node->outsock[i], &heartbeat_message, sizeof(message_t)) == -1){
                        leadlog("ERROR", "Error while sending heartbeat to %s:%d. (%s)",
                                inet_ntoa(hacluster->node_addresses[i].sin_addr), 
                                hacluster->node_addresses[i].sin_port,
                                strerror(errno)
                        );
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

    // ignore messages with lower term
    if(s_term < node->shared->current_node_term){
        leadlog("INFO", "As follower - %d term of incoming message less that %ld", s_term, node->shared->current_node_term); 
        RETURN_SUCCESS();
    }

    switch (type)
    {

    case Heartbeat:
        leadlog("INFO", "As follower - got message type of Hearbeat from %d with %d term", s_id, s_term);

        // handle what leader changes and assign new term if incoming one is more
        node->shared->leader_id = s_id;
        if(s_term > node->shared->current_node_term){
            node->shared->current_node_term = s_term;
        }

        break;

    case ElectionRequest:
        leadlog("INFO", "As follower - got message type of ElectionRequest from %d with %d term", s_id, s_term);

        // vote for node if we haven't voted in the term (if() condition guarantees this)
        if(s_term > node->shared->current_node_term){
            message_t response;

            node->shared->current_node_term = s_term;
            
            response.type = ElectionResponse;
            response.sender_id = node->node_id;
            response.sender_state = node->state;
            response.sender_term = node->shared->current_node_term;

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

    // ignore messages with lower term
    if(s_term < node->shared->current_node_term){
        leadlog("INFO", "As candidate - %d term of incoming message less that %ld", s_term, node->shared->current_node_term); 
        RETURN_SUCCESS();
    }

    switch (type)
    {
    case ElectionRequest:
        leadlog("INFO", "As candidate - got ElectionRequest with %d term (current %ld)", s_term, node->shared->current_node_term); 
        if(s_term > node->shared->current_node_term){
            node->shared->current_node_term = s_term;
            GOTO_follower(node);
        }
        break;
    case Heartbeat:
        leadlog("INFO", "As candidate - got Heatbeat with %d term (current %ld)", s_term, node->shared->current_node_term); 
        if(s_term >= node->shared->current_node_term){
            node->shared->current_node_term = s_term;
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

    // ignore messages with lower term
    if(s_term < node->shared->current_node_term){
        RETURN_SUCCESS();
    }

    switch (type)
    {
    case ElectionRequest:
        leadlog("INFO", "As leader - got ElectionRequest (sender term %d, current term %ld)", s_term, node->shared->current_node_term); 
        if(s_term > node->shared->current_node_term){
            node->shared->current_node_term = s_term;
            GOTO_follower(node);
        }
        break;
    case Heartbeat:
        leadlog("INFO", "As leader - got Heartbeat (sender term %d, current term %ld)", s_term, node->shared->current_node_term);  
        if(s_term >= node->shared->current_node_term){
            node->shared->current_node_term = s_term;
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


