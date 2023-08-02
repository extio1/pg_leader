#!/bin/bash

# psql connects to the node by UNIX-socket
# that's why network namespaces doesn't broke everything

source ../config/pg_leader.config

ID=$1

if [ -z "$ID" ]; then
    echo "Please enter the ID of the node."
    exit
fi

PORT=$(($PGLD_START_PORT + $ID))    

watch -n1 -t "echo 'SELECT get_node_info();' | psql postgres -p $PORT" 
