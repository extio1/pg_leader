#include "../include/pgha.h"
#include "../include/ha_types.h"
#include "../include/parser.h"

node_t* node = NULL;
cluster_t* cluster = NULL;

PGDLLEXPORT error_code_t
node_init(Datum datum)
{

    node = malloc(sizeof(node_t));
    cluster = malloc(sizeof(cluster_t));

    if(node == NULL || cluster == NULL){
        return MALLOC_ERROR; 
    } 
    
    if( parse_cluster_config("../config/cluster.config", cluster) != SUCCESS){
        
    }
    
    if( parse_node_config("../config/node.config", node) != SUCCESS ){

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
