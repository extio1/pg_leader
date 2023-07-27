#!/bin/bash

# Supports up to 246 nodes in it own namespace

N_NODES=$1
network=192.168.10
mask=/24

# Create $N_NODES network namespaces 
for (( i=0; i<N_NODES; i++ ))
do 
    sudo ip netns add node"$i"_netns
done

echo $N_NODES network namespaces created:
tree /var/run/netns/

# Set up $N_NODES vitrual ethernet interfaces for each node.
for (( i=0; i<N_NODES; i++ ))
do 
    #create
    sudo ip link add veth"$i" type veth peer name ceth"$i"
    #up one end in default namespace
    sudo ip link set veth"$i" up
    #connect another to namespace
    sudo ip link set ceth"$i" netns node"$i"_netns
done

# In each network namespace set up interfaces and assign ip address
for (( i=0; i<N_NODES; i++ ))
do 
    sudo ip netns exec node"$i"_netns ip link set lo up
    sudo ip netns exec node"$i"_netns ip link set ceth"$i" up
    ((addr=$i+10))
    sudo ip netns exec node"$i"_netns ip addr add "$network"."$addr""$mask" dev ceth"$i"
done


BRIDGE_NAME=br0

# Create and set up bridge
sudo ip link add name br0 type bridge
sudo ip addr add "$network".1"$mask" dev "$BRIDGE_NAME"
sudo ip link set "$BRIDGE_NAME" up

# Connect all ends in the root namespace to the bridge
for (( i=0; i<N_NODES; i++ ))
do 
    sudo ip link set veth"$i" master "$BRIDGE_NAME"
done

bridge link show br0

echo "Network initialization done."