#ifndef RW_FIREWALL_H
#define RW_FIREWALL_H

#include <pthread.h>

extern pthread_t firewall_thread;

extern pthread_mutex_t message_buffer_mutex;
extern pthread_cond_t message_buffer_conditional;

void* firewall_routine(void*);

#endif /* RW_FIREWALL_H */