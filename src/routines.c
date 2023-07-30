#include "../log/logger.h"
#include "../include/error.h"
#include "../include/ld_types.h"
#include "../include/message.h"
#include "../include/parser.h"
#include "../include/pgld.h"
#include "../include/firewall.h"

#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <time.h>
#include <stdbool.h>
#include <sys/time.h>
#include <stdlib.h>
#include <unistd.h>


#define SUCCESS -100
#define TIMEOUT -101

/* --- Income message handlers for each state --- */

static pl_error_t handle_message_follower(void);
static pl_error_t handle_message_candidate(unsigned int*);
static pl_error_t handle_message_leader(void);

static int read_message(const int min_timeout, const int max_timeout);

static volatile int message_buffer_ready = 0;

void* 
firewall_routine(void* args){
    message_t* buffer = malloc(sizeof(message_t));

    while(1){
        int red = read(node->insock, buffer, sizeof(message_t));
        
        struct timeval tv;
        socklen_t len;
        getsockopt(node->insock, SOL_SOCKET, SO_RCVTIMEO, &tv, &len);

        elog(LOG, "Firewall reads from %d %d (%ld, %ld)", buffer->sender_id, red, tv.tv_sec, tv.tv_usec);

        if(red < 0) {
            elog(LOG, "firewall reading error");
        } 
        else 
        {
            if(message_buffer->sender_term >= node->shared->current_node_term){
                if(pthread_mutex_trylock(&message_buffer_mutex) == 0){ // if the mutex was acquired
                    leadlog("INFO", "FIREWALL TAKES MUTEX");
                    memcpy(message_buffer, buffer, sizeof(message_t));
                    message_buffer_ready = 1;
                    if (pthread_cond_signal(&message_buffer_conditional) != 0){
                        leadlog("INFO", "pthread_cond_signal error");
                    }
                    pthread_mutex_unlock(&message_buffer_mutex);
                    leadlog("INFO", "FIREWALL UNLOCKs Mutex");
                }
            }
        }
    }
}

void main_cycle(void){
    while(1){
        MODERATE_HANDLE(routine());
    }
}

pl_error_t
follower_routine(void)
{   

    while(1)
    {
        int read_code = read_message(node->min_timeout, node->max_timeout);

        if(read_code == TIMEOUT)
        {
            GOTO_candidate(node);
        } else if(read_code == SUCCESS) 
        {
            MODERATE_HANDLE(handle_message_follower());
        } else 
        {
            THROW(READ_ERROR, "Follower read error: %s", strerror(read_code));
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
    for(int i = 0; i < hacluster->n_nodes; ++i)
    {
        if(i == node->node_id)
            continue;

        leadlog("INFO", "As candidate - sending message to %s:%d. (%ld term)", 
            inet_ntoa(hacluster->node_addresses[i].sin_addr), hacluster->node_addresses[i].sin_port,
            node->shared->current_node_term);

        if( write(node->outsock[i], &elect_req, sizeof(message_t)) == -1 )
            leadlog("ERROR", "As candidate - sending message to %s:%d error (%s)", 
                    inet_ntoa(hacluster->node_addresses[i].sin_addr), hacluster->node_addresses[i].sin_port,
                    strerror(errno));
    }

    // wait for responces
    while(1){
        int error_code = read_message(node->min_timeout, node->max_timeout);

        if(error_code == TIMEOUT)
        {
            GOTO_candidate(node);
        } else if(error_code == SUCCESS) 
        {
            node_state_t prev_state = node->state;

            MODERATE_HANDLE(handle_message_candidate(&electorate_size));

            // if handle_message_candidate() changes node state
            // then make return from that state
            if(prev_state != node->state){
                RETURN_SUCCESS();
            }
        } else 
        {
            THROW(READ_ERROR, "Candidate read error %s", strerror(error_code));
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

    while(1)
    {
        // if no message got while heartbeat_timeout send heartbeat_message,
        // otherwise handle got message
        leadlog("INFO", "As leader - read start."); 
        int error_code = read_message(node->heartbeat_timeout, node->heartbeat_timeout);
        if( error_code == TIMEOUT ) {
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
        } else if( error_code == SUCCESS)
        {
            node_state_t prev_state = node->state;

            MODERATE_HANDLE(handle_message_leader());

            // if handle_message_leader() changes node state
            // then make return from that state
            if(prev_state != node->state){
                RETURN_SUCCESS();
            }
        } else 
        {
            THROW(READ_ERROR, "Leader read error %s", strerror(error_code));
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

int read_message(const int min_timeout, const int max_timeout){
    //assing new random timeout in range
    int err = 0;
    struct timespec time_temp;
    const unsigned long long ms_in_sec = 1000000000LL; // 10^9 
    leadlog("INFO", "calc extra start");
    int diff = max_timeout-min_timeout;
    unsigned long extra = (diff > 0) ? (min_timeout + rand()%(diff)) : min_timeout;
    leadlog("INFO", "extra %ld", extra);

    if(clock_gettime(CLOCK_REALTIME, &time_temp) != 0){
        leadlog("ERROR", "clock_gettime error (%s)", strerror(errno));
    }

    time_temp.tv_nsec += extra % ms_in_sec;
    time_temp.tv_sec += (time_temp.tv_nsec / ms_in_sec) + 1;
    time_temp.tv_nsec %= ms_in_sec;

    leadlog("INFO", "sec %ld nsec %ld", time_temp.tv_sec, time_temp.tv_nsec);    

    leadlog("INFO", "Waiting for messages #1");

    pthread_mutex_lock(&message_buffer_mutex);
    leadlog("INFO", "Waiting for messages #2 (mutex aciquired)");
    while(!message_buffer_ready && err != ETIMEDOUT){
        err = pthread_cond_timedwait(&message_buffer_conditional, &message_buffer_mutex, &time_temp);
    }

    message_buffer_ready = 0;
    pthread_mutex_unlock(&message_buffer_mutex);

    if( err == 0 ){
        leadlog("INFO", "Waiting for messages #3 (got message)");
        return SUCCESS;
    } else if (err == ETIMEDOUT){
        leadlog("INFO", "Waiting for messages #3 (timeout)");
        return TIMEOUT;
    } else {
        leadlog("ERROR", "Waiting for messages #3 (%s)", strerror(err));
        return err;
    }
}