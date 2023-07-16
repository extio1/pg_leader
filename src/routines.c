#include "../include/ha_types.h"
#include "../include/parser.h"
#include "../include/pgha.h"
#include "../include/error.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


cluster_t* hacluster = NULL;
node_t* node = NULL;

error_code_t follower_routine(void);
error_code_t candidate_routine(void);
error_code_t leader_routine(void);

PGDLLEXPORT error_code_t
node_routine(Datum datum)
{
    error_code_t error;

    
    hacluster = malloc(sizeof(cluster_t));    
    node = malloc(sizeof(node_t));

    if(node == NULL || hacluster == NULL){
        return MALLOC_ERROR; 
    }  

    if( (error = parse_cluster_config("pgha_config/cluster.config", hacluster)) != SUCCESS){
        print_error_info(error, strerror(errno));
        return error;
    }

    if( (error = parse_node_config("pgha_config/node.config", node)) != SUCCESS ){
        print_error_info(error, strerror(errno));
        return error;
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
