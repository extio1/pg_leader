#include "../include/parser.h"

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>

#include "postgres.h"


pl_error_t 
parse_cluster_config(const char* path, cluster_t* cluster){
    FILE* config = fopen(path, "r");

    size_t n_nodes = 0;
    char in_addr[15];
    char port[5];

    char* buff = malloc(128);
    size_t n = 128;

    int i = 0;

    if(config == NULL){
        POSIX_THROW(FOPEN_ERROR);
    }
    
   
    while(getline(&buff, &n, config) != -1){
        if(buff[0] != '#' && buff[0] != '\n'){
            ++n_nodes;
        }
    }

    cluster->node_addresses = malloc(sizeof(socket_node_info_t)*n_nodes);

    elog(LOG, "%ld nodes detected in config (%s) file.", n_nodes, path);

    if(cluster->node_addresses == NULL){
        POSIX_THROW(MALLOC_ERROR);
    }

    fseek(config, 0, SEEK_SET);
    while(getline(&buff, &n, config) != -1){
        if(buff[0] != '#' && buff[0] != '\n'){
            struct sockaddr_in addr;

            sscanf(buff, "%s %s", in_addr, port);

            if(inet_aton(in_addr, &addr.sin_addr) == 0){
                POSIX_THROW(WRONG_IPADDR_ERROR);
            }
            addr.sin_port = atol(port);

            cluster->node_addresses[i] = addr;
            elog(LOG, "#%d node added: %s:%d", i, inet_ntoa(addr.sin_addr), addr.sin_port);
            ++i;
        }
    }

    cluster->n_nodes = n_nodes;

    fclose(config);
    free(buff);

    RETURN_SUCCESS();
}

pl_error_t 
parse_node_config(const char* path, node_t* node){
    FILE* config = fopen(path, "r");

    int id = 0;
    char* buff = malloc(128);
    size_t n = 128;

    if(config == NULL){
        POSIX_THROW(FOPEN_ERROR);
    }

    while(getline(&buff, &n, config) != -1){
        if(buff[0] != '#' && buff[0] != '\n'){
            sscanf(buff, "%d", &id);

            node->node_id = id;
            elog(LOG, "node assigned id #%d", node->node_id);
        }
    }


    fclose(config);
    free(buff);

    RETURN_SUCCESS();
}

pl_error_t
parse_timeout_config(const char* path, node_t* node)
{
    FILE* config = fopen(path, "r");

    char* buff = malloc(128);
    char key[32];
    char value[16];
    size_t n = 128;

    if(config == NULL){
        POSIX_THROW(FOPEN_ERROR);
    }

    while(getline(&buff, &n, config) != -1){
        sscanf(buff, "%s = %s", key, value);
        if(strcmp("min_timeout", key) == 0){
            node->min_timeout = atoi(value);
        }
        else if(strcmp("max_timeout", key) == 0){
            node->max_timeout = atoi(value);
        }
        else if(strcmp("hearbeat_period", key) == 0){
            node->heartbeat_timeout = atoi(value);
        }
    }


    fclose(config);
    free(buff);

    RETURN_SUCCESS();
}