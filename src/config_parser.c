#include "../include/parser.h"

#include <arpa/inet.h>
#include <stdio.h>


error_code_t 
parse_cluster_config(const char* path, cluster_t* cluster){
    FILE* config = fopen(path, "r");

    int id;
    char id_addr[15];
    char port[5];
    int assigned = 0;

    if(config == NULL){
        return FOPEN_ERROR;
    }

    while( (assigned = fscanf(config, "%d:%s:%s", &id, id_addr, port)) != EOF) {
        if(assigned != 3){
            continue;
        } else {
            struct sockaddr_in addr;
            addr.sin_addr.s_addr = inet_aton(id_addr);
            if(addr.sin_addr.s_addr == 0){
                return WRONG_IPADDR_ERROR;
            }
            addr.sin_port = port;
            
            insert(&cluster->node_addresses, id, addr);
        }
    };

    fclose(config);

    return SUCCESS;
}

error_code_t 
parse_node_config(const char* path, node_t* node){
    FILE* config = fopen(path, "r");

    int assigned = 0;
    int id = 0;

    if(config == NULL){
            return FOPEN_ERROR;
    }

    while( (assigned = fscanf(config, "%d", &id)) != EOF) {
        if(assigned != 3){
            continue;
        } else {
            node->node_id = id;
        }
    };


    fclose(config);

    return SUCCESS;
}