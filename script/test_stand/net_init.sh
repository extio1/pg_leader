#!/bin/bash

source $PGLD_PATH/config/pg_leader.config

# Create $N_NODES network namespaces 
for (( i=0; i<N_NODES; i++ ))
do 
    sudo ip netns add $PGLD_NNS_PREFIX"$i"
done

echo $N_NODES network namespaces created:
tree /var/run/netns/

# Set up $N_NODES vitrual ethernet interfaces for each node.
for (( i=0; i<N_NODES; i++ ))
do 
    #create
    sudo ip link add $VETH_ROOT_NAME_PREFIX"$i" type veth peer name $VETH_LOCAL_NAME_PREFIX"$i"
    #up one end in default namespace
    sudo ip link set $VETH_ROOT_NAME_PREFIX"$i" up
    #connect another to namespace
    sudo ip link set $VETH_LOCAL_NAME_PREFIX"$i" netns $PGLD_NNS_PREFIX"$i"
done

# In each network namespace set up interfaces and assign ip address
for (( i=0; i<N_NODES; i++ ))
do 
    sudo ip netns exec $PGLD_NNS_PREFIX"$i" ip link set lo up
    sudo ip netns exec $PGLD_NNS_PREFIX"$i" ip link set $VETH_LOCAL_NAME_PREFIX"$i" up
    ((addr=$i+10))
    sudo ip netns exec $PGLD_NNS_PREFIX"$i" ip addr add "$NETWORK"."$addr"/"$MASK" dev $VETH_LOCAL_NAME_PREFIX"$i"
done

# Create and set up bridge
sudo ip link add name $BRIDGE_NAME type bridge
sudo ip addr add "$NETWORK".1/"$MASK" dev "$BRIDGE_NAME"
sudo ip link set "$BRIDGE_NAME" up

# Connect all ends in the root namespace to the bridge
for (( i=0; i<N_NODES; i++ ))
do
    sudo ip link set $VETH_ROOT_NAME_PREFIX"$i" master "$BRIDGE_NAME"
done

bridge link show $BRIDGE_NAME

echo "Network initialization done."
