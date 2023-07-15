#include "../include/error.h"

#include <glib.h>
#include <sys/socket.h>
#include <netinet/in.h>

/*
 * For using a different implementation of hashtable change typedef ... hash_table
 * and implement it including this header
 */

typedef GHashTable hash_table;

/*
 * For using different key_t, value_t change these typedefs
 */
typedef int key_t;
typedef struct sockaddr_in value_t;

hash_table* init(void);
error_code_t insert(hash_table*, key_t, value_t);
value_t get(hash_table*, key_t);
