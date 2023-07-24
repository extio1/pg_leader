#include "../include/pgld.h"

#include <fmgr.h>
#include <postgres.h>
#include <utils/palloc.h>

/* --- SQL used function to display info ---  */

PG_FUNCTION_INFO_V1(get_node_info);
PG_FUNCTION_INFO_V1(get_cluster_config);

Datum
get_cluster_config(PG_FUNCTION_ARGS)
{
    PG_RETURN_CSTRING(
        "not implemented" 
    );
}

Datum
get_node_info(PG_FUNCTION_ARGS)
{   
    char* buffer = palloc(256);
    
    sprintf(buffer, "leader - %d; node #%d, term #%ld", 
            shared_info_node->leader_id, shared_info_node->node_id, 
            shared_info_node->current_node_term);

    PG_RETURN_CSTRING(buffer);
}