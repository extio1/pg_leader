#ifndef MESSAGE_H
#define MESSAGE_H

#include "ld_types.h"

#include <sys/time.h>

typedef enum message_type
{
    Heartbeat,
    ElectionRequest,
    ElectionResponse,
} message_type_t;


typedef struct message
{
    term_t sender_term;
    node_id_t sender_id;
    node_state_t sender_state;
    message_type_t type;
} message_t;

#endif /* MESSAGE_H */