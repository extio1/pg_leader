#include "../log/logger.h"
#include "../include/error.h"
#include "../include/ld_types.h"
#include "../include/message.h"
#include "../include/parser.h"
#include "../include/pgld.h"
#include "../include/node.h"

#include <arpa/inet.h>
#include <errno.h>
#include <time.h>
#include <stdbool.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>

#define READ_SUCCESS -100
#define READ_TIMEOUT -101
#define READ_ERROR   -102

/* --- Income message handlers for each state --- */

static pl_error_t handle_message_follower(void);
static pl_error_t handle_message_candidate(unsigned int*);
static pl_error_t handle_message_leader(void);

// Returns SUCCESS, TIMEOUT or ERROR. If ERROR, errno variable is set to indicate the error.
static int read_message(const int min_timeout, const int max_timeout);

void main_cycle(void){
    while(1){
        SAFE(routine());
    }
}

pl_error_t
follower_routine(void)
{   
    while(1){
        int error = read_message(node->min_timeout, node->max_timeout);

        if(error == READ_SUCCESS)
        {
            SAFE(handle_message_follower());
        } else if (error == READ_TIMEOUT) 
        {
            leadlog("INFO", "As follower - timeout");
            GOTO_candidate(node);
        } else 
        {
            THROW(READ_ERROR, "Follower error (%s)", strerror(errno));
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
    elect_req.sender_id = node->shared->node_id;
    elect_req.sender_state = node->state;
    elect_req.sender_term = node->shared->current_node_term;

    // send everyone except for itself election request
    for(int i = 0; i < hacluster->n_nodes; ++i){
        if(i != node->shared->node_id){
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
        int error = read_message(node->min_timeout, node->max_timeout);
        // timeout while waiting responces

        if(error == READ_SUCCESS)
        {
            node_state_t prev_state = node->state;

            SAFE(handle_message_candidate(&electorate_size));

            // if handle_message_candidate() changes node state
            // then make return from that state
            if(prev_state != node->state){
                RETURN_SUCCESS();
            }
        } else if(error == READ_TIMEOUT)
        {
            leadlog("INFO", "As candidate - timeout."); 
            GOTO_candidate(node);
        } else 
        {
            THROW(READ_ERROR, "Candidate error (%s)", strerror(errno));
        }
    }
}

pl_error_t
leader_routine(void)
{   
    struct timeval heartbeat_timeout = { .tv_sec = 0, .tv_usec = node->heartbeat_timeout };
    message_t heartbeat_message;

    node->shared->leader_id = node->shared->node_id; // this node is a leader now

    heartbeat_message.type = Heartbeat;
    heartbeat_message.sender_id = node->shared->node_id;
    heartbeat_message.sender_state = node->state;
    heartbeat_message.sender_term = node->shared->current_node_term;

    while(1){
        // if no message got while heartbeat_timeout send heartbeat_message,
        // otherwise handle got message
        int error = read_message(node->heartbeat_timeout , node->heartbeat_timeout);

        if( error == READ_SUCCESS ) 
        {
            node_state_t prev_state = node->state;

            SAFE(handle_message_leader());

            // if handle_message_leader() changes node state
            // then make return from that state
            if(prev_state != node->state){
                RETURN_SUCCESS();
            }
        } else if( error == READ_TIMEOUT) 
        {
            leadlog("INFO", "As leader - timeout, make heartbeat %ld.", heartbeat_timeout.tv_usec); 

            for(int i = 0; i < hacluster->n_nodes; ++i){ // send everyone heartbeat (except for itself)
                if(i == node->shared->node_id){
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
        } else 
        {
            THROW(READ_ERROR, "Leader error (%s)", strerror(errno));
        }
    }

}

pl_error_t
handle_message_follower()
{
    int s_id = message_buffer->sender_id;
    int s_term = message_buffer->sender_term;

    switch (message_buffer->type)
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
            response.sender_id = node->shared->node_id;
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
    int s_term = message_buffer->sender_term;

    switch (message_buffer->type)
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
    int s_term = message_buffer->sender_term;

    switch (message_buffer->type)
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
             "node in leader state can not handle %d type of message", message_buffer->type);
        break;
    }

    RETURN_SUCCESS();
}

int 
read_message(const int min_timeout, const int max_timeout)
{
    struct timeval timeout, start, now;
    int diff = max_timeout - min_timeout;                                 
    timeout.tv_sec = 0;                                                                                                                                         
    timeout.tv_usec = (diff > 0) ? (min_timeout + rand()%(diff)) : min_timeout; 

wait_for_messages:
    if( setsockopt(node->insock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(struct timeval)) == -1){     
        return ERROR;                             
    }

    if( gettimeofday(&start, NULL) != 0 )
       return ERROR;

    if( read(node->insock, message_buffer, sizeof(message_t)) == -1 ) {
        if(errno = EWOULDBLOCK){
            return READ_TIMEOUT;
        } else {
            return READ_ERROR;
        }
    } else {
        if(message_buffer->sender_term >= node->shared->current_node_term){
            return READ_SUCCESS;
        } else {
            if( gettimeofday(&now, NULL) != 0 ){
                return READ_ERROR;
            }
            timeout.tv_sec -= now.tv_sec-start.tv_sec;
            timeout.tv_usec -= now.tv_usec-start.tv_usec;
            if(timeout.tv_sec >= 0 && timeout.tv_usec > 0){
                goto wait_for_messages;
            } else {
                return READ_TIMEOUT;
            }
        }
       return READ_SUCCESS;
    }
}
