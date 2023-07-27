#!/bin/bash

N_NODES=$1

INTERFACE_NAME=veth

EXTENSION_NAME=pg_leader  #param1
PATHDB=/home/extio1/db    #param2
POSTGRES_BIN_PATH=$(pg_config --bindir)

PORT_START=5000

function help {
    echo "+---------------------------------------------------------------------------------------------+"
    echo " 0) h, help, h, ? - help."
    echo " 1) st, start [ALL | NODE IDs] - pg_ctl start all (some) nodes."
    echo " 2) sp, stop [ALL | NODE IDs] - pg_ctl stop all (some) nodes."
    echo " 3) d, disconnect [-b] ONE ANOTHER - prohibit sending packets from ONE to ANOTHER."
    echo "    with -b parameter prohibit sending in both directs."
    echo " 4) r, reconnect [-b] ONE ANOTHER - vice versa as disconnect."
    echo " 5) b, brainsplit FIRST_GROUP_IDs SECOND_GROUP_IDs - split into two groups where no one node from"
    echo "    FIRST group cannot send packets to someone in SECOND group and vice versa."
    echo " 6) statistics, stat - network rools stat"
    echo " 7) f, flush - removes all network restrictions"
    echo " 8) exit, q, quit - exit the script"
    echo "+---------------------------------------------------------------------------------------------+"
}

function start {
    REQ_STR=""
    if [[ $2 == "all" ]]; then
        for (( i=0; i<N_NODES; i++ ))
        do
            REQ_STR+="$i"" "
        done
    else
        REQ_STR=$@
    fi

    read -a NODE_LAUNCH_ARRAY <<< $REQ_STR

    for NODE in "${NODE_LAUNCH_ARRAY[@]}"
    do
        NAMESPACE_NAME=node"$NODE"_netns
        if [ -f /var/run/netns/$NAMESPACE_NAME ]; then
            ((PORT=$PORT_START+$NODE))
            ##echo "shared_preload_libraries=$EXTENSION_NAME" >> $PATHDB/postgresql.conf
            echo $PATHDB/db$NODE, $NAMESPACE_NAME: starting
            sudo ip netns exec $NAMESPACE_NAME bash -c "sudo -u $USER $POSTGRES_BIN_PATH/pg_ctl -D $PATHDB/db$NODE -l $PATHDB/logfile$NODE -o \"-c shared_preload_libraries=$EXTENSION_NAME -p $PORT\" start"
            echo
        else
            echo "$NAMESPACE_NAME network namespace isn't created. Please launch net_init.sh."
            echo
        fi
    done
}

function stop {
    REQ_STR=""
    if [[ $2 == "all" ]]; then
        for (( i=0; i<N_NODES; i++ ))
        do
            REQ_STR+="$i"" "
        done
    else
        REQ_STR=$@
    fi

    read -a NODE_LAUNCH_ARRAY <<< $REQ_STR

    for NODE in "${NODE_LAUNCH_ARRAY[@]}"
    do
        NAMESPACE_NAME=node"$NODE"_netns
        if [ -f /var/run/netns/$NAMESPACE_NAME ]; then
            ((PORT=$PORT_START+$NODE))
            ##echo "shared_preload_libraries=$EXTENSION_NAME" >> $PATHDB/postgresql.conf
            echo $PATHDB/db$NODE, $NAMESPACE_NAME: stopping
            sudo ip netns exec $NAMESPACE_NAME bash -c "sudo -u $USER $POSTGRES_BIN_PATH/pg_ctl -D $PATHDB/db$NODE stop"
            echo
        else
            echo "$NAMESPACE_NAME network namespace isn't created. Please launch net_init.sh."
            echo
        fi
    done
}

function disconnect() {
    if [[ $# != 3 ]]; then
        echo "Please enter 2 node ids"
        return
    fi

    if (( !( 0<=$2 && $2<$N_NODES && 0<=$3 && $3<$N_NODES ) )); then
        echo "Both node ids should in range [0, $N_NODES)"
        return
    fi

    if (( $2 == $3 )); then
        echo "IDs should be different"
        return
    fi

    sudo ebtables -A FORWARD -i $INTERFACE_NAME"$2" -o $INTERFACE_NAME"$3" -j DROP

    echo $INTERFACE_NAME"$2" "-x->" $INTERFACE_NAME"$3"
}

function reconnect() {
    if [[ $# != 3 ]]; then
        echo "Please enter 2 node ids"
        return
    fi

    if (( !( 0<=$2 && $2<$N_NODES && 0<=$3 && $3<$N_NODES ) )); then
        echo "Both node ids should in range [0, $N_NODES)"
        return
    fi

    if (( $2 == $3 )); then
        echo "IDs should be different"
        return
    fi

    sudo ebtables -D FORWARD -i $INTERFACE_NAME"$2" -o $INTERFACE_NAME"$3" -j DROP

    echo $INTERFACE_NAME"$2" "--->" $INTERFACE_NAME"$3"
}

function brainsplit() {
    if (( $# != $N_NODES+1 )); then
        echo "Warning: num of input params less than $N_NODES"
    fi
    
    a=$*
    b=$1
    GROUPS_STR="${a/b/""}"

    IFS=";" read -a GROUPS_ARRAY <<< $GROUPS_STR

    read -a GROUP1 <<< ${GROUPS_ARRAY[0]}
    read -a GROUP2 <<< ${GROUPS_ARRAY[1]}

    for NODE1 in "${GROUP1[@]}"
    do
        for NODE2 in "${GROUP2[@]}"
        do
            sudo ebtables -A FORWARD -i $INTERFACE_NAME"$NODE1" -o $INTERFACE_NAME"$NODE2" -j DROP
            sudo ebtables -A FORWARD -i $INTERFACE_NAME"$NODE2" -o $INTERFACE_NAME"$NODE1" -j DROP
        done
    done 

    echo "{"${GROUPS_ARRAY[0]}" }" "-x->" "{"${GROUPS_ARRAY[1]}" }"
}

function stat() {
    echo -e "\033[1mNodes status:\033[0m"

    for (( NODE=0; NODE<N_NODES; NODE++))
    do    NAMESPACE_NAME=node"$NODE"_netns
        if [ -f /var/run/netns/$NAMESPACE_NAME ]; then
            echo $PATHDB/db$NODE, $NAMESPACE_NAME: status
            sudo ip netns exec node"$NODE"_netns bash -c "sudo -u $USER $POSTGRES_BIN_PATH/pg_ctl -D $PATHDB/db$NODE status"
            echo
        else
            echo "$NAMESPACE_NAME network namespace isn't created. Please launch net_init.sh."
            echo
        fi
    done

    echo -e "\033[1mNetwork status:\033[0m"

    sudo ebtables -L FORWARD
}

function flush() {
    sudo ebtables --flush FORWARD
    echo "Flushed"
}



####################### main #######################

if [ -z "$N_NODES" ]; then
    echo "Please enter the num of nodes."
    exit
fi

# Create if not exists directory with database clusters
if [[ !(-d "$PATHDB") ]]; then
    mkdir $PATHDB
fi
echo "Directory $PATHDB EXISTS." 

# Create if not exists database clusters
for (( i=0; i<N_NODES; i++ ))
do
    PATHDB_temp=$PATHDB/db"$i" 
    if [[ !(-d "$PATHDB") ]]; then
        initdb -D $PATHDB_temp
        echo "$PATHDB_temp database cluster CREATED."
    else
        echo "$PATHDB_temp database cluster EXISTS."
    fi
done

help
#main cycle
while :
do 
    read -p "$USER/pg_leader #) " READ_TMP
    READ_TMP=${READ_TMP,,}

    if [[ $READ_TMP == "help"* || $READ_TMP == "h"* || $READ_TMP == "1"* || $READ_TMP == "?"* ]]; then
        help
    elif [[ $READ_TMP == "start "* || $READ_TMP == "1 "* || $READ_TMP == "st "* ]]; then
        start $READ_TMP
    elif [[ $READ_TMP == "stop "* || $READ_TMP == "2 "* || $READ_TMP == "sp "* ]]; then
        stop $READ_TMP
    elif [[ $READ_TMP == "disconnect "* || $READ_TMP == "3 "* || $READ_TMP == "d "* ]]; then
        disconnect $READ_TMP
    elif [[ $READ_TMP == "reconnect "* || $READ_TMP == "4 "* || $READ_TMP == "r "* ]]; then
        reconnect $READ_TMP
    elif [[ $READ_TMP == "brainsplit "* || $READ_TMP == "5 "* || $READ_TMP == "b "*  ]]; then
        brainsplit $READ_TMP 
    elif [[ $READ_TMP == "stat"*  || $READ_TMP == "statistics"* || $READ_TMP == "6"* ]]; then
        stat
    elif [[ $READ_TMP == "f"*  || $READ_TMP == "flush"* || $READ_TMP == "7 "* ]]; then
        flush
    elif [[ $READ_TMP == "exit"* || $READ_TMP == "8"* || $READ_TMP == "q"* || $READ_TMP == "quit"* ]]; then
        exit
    fi

done

####################### main #######################