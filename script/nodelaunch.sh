#!/bin/bash

source ../config/pg_leader.config

ID=$1

if [ -z "$ID" ]; then
    echo "Please enter the ID of the node."
    exit
fi

PORT=$(($PGLD_START_PORT + $ID))

echo $ID

postgres -D "$PGLD_DB_PATHNAME_PREFIX"$ID                                                                 \
             -c shared_preload_libraries=$EXTENSION_NAME                                                  \
             -c pg_leader.node_id=$ID                                                                     \
             -c pg_leader.n_nodes=$N_NODES                                                                \
             -c pg_leader.cluster_conf="$IPv4_CLUSTER"                                                    \
             -c pg_leader.min_timeout=$MIN_TIMEOUT                                                        \
             -c pg_leader.max_timeout=$MAX_TIMEOUT                                                        \
             -c pg_leader.heartbeat_timeout=$HEARTBEAT_TIMEOUT                                            \
             -c pg_leader.enable_log=$ENABLE_LOG                                                          \
             -c pg_leader.log_file=$LOG_NAME                                                              \
             -p $PORT
