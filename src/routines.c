#include "../include/hashtable.h"
#include "../include/ha_types.h"
#include "../include/parser.h"
#include "../include/pgha.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


node_t* node = NULL;
cluster_t* cluster = NULL;

error_code_t follower_routine(void);
error_code_t candidate_routine(void);
error_code_t leader_routine(void);

PGDLLEXPORT error_code_t
node_routine(Datum datum)
{
    error_code_t error;

    node = malloc(sizeof(node_t));
    cluster = malloc(sizeof(cluster_t));

    if(node == NULL || cluster == NULL){
        return MALLOC_ERROR; 
    } 
    
    init(cluster->node_addresses);

    if( (error = parse_cluster_config("../config/cluster.config", cluster)) != SUCCESS){
        return error;
    }
    
    if( (error = parse_node_config("../config/node.config", node)) != SUCCESS ){
        return error;
    }

    for(int i = 0; i < cluster->n_nodes; ++i){
        printf("%d %s\n", i, inet_ntoa(get(cluster->node_addresses, &i)->sin_addr));
    }

    return SUCCESS;
}

error_code_t
follower_routine(void)
{
    return SUCCESS;
}

error_code_t
candidate_routine(void)
{
    return SUCCESS;
}

error_code_t
leader_routine(void)
{   
    return SUCCESS;
}
