# pg_leader
PostgreSQL extension. RAFT-based algorithm realization of consensus algorithm of choosing the leader.   
The job implements the algorithm of choosing the leader. It has scripts to observe which one ID is leader according to the opinion of some node (the aim that every node has the same opinion). Besides, there is a test stand allows to in interactive mode, by writing your own script control the condition of each node (turned on/off) and network between them. 

## Contents
- [/config](./config/pg_leader.config) contains the configuration file of pg_leader extension.

- [/script](./script) contains scripts are using for launching node with pg_leader extension, for observing it condition via the shell. Also [./script/test_stand](./script/test_stand) has the test stand. Scripts are written on bash, each name of executable starts with pgld_*.
    - **_pgld_launchnode_** for launching the node (postgres+pg_leader)
    - **_pgld_watchnode_** for observing node condition: it **node ID**, **term** and **leader ID** on current node (leader ID may probably be different on nodes, the aim of this extension that each node has the same leader ID)
    - **_test_stand/pgld_stnd_netinit.sh_** are using for initialization network namespaces and network between them. 
    - **_test_stand/pgld_stnd_ctl_** are using for interactive control of the test stand.
    - **_test_stand/pgld_stnd_monitor_** TODO

## Installation
>Launching nodes, cluster observing and launching test stand may be carried out using scripts from ./script directory.

### 0. Clone the repository to the arbitrary directory 
``$ git clone https://gitpglab.postgrespro.ru/t.shalnev/pg_leader.git``
### 1. Environment prepare.
`PGLD_PATH` prepare:   
``$ export PGLD_PATH=/path/to/cloned/directory/pg_leader/``       
As well you should have exported to `PATH` /bin directory of postgres:    
``$ export PATH=/path/to/postgres/bin/:$PATH``  
To use scripts from any place do:   
``$ export PATH=$PGLD_PATH/script/:$PGLD_PATH/script/test_stand/:$PATH``
```shell
export PGLD_PATH=/path/to/cloned/directory/pg_leader/
export PATH=/path/to/postgres/bin/:$PATH
export PATH=$PGLD_PATH/script/:$PGLD_PATH/script/test_stand/:$PATH
```
For more comfortable use you may append these strings to your shell configuration file - for instance, `~/.bashrc` for Bash, to have these exports in each shell launch.

### 2. Building.
``$ cd $PGLD_PATH``    
And launch `Makefile` in pg_leader directory  
``$ make install``  
**Now we have the extension installed above Postgres.**

### 3. [Config file](./config/pg_leader.config) editing.
> Config file is out of the box ready to launch 5 nodes using test stand.   

Most important parameters:  
* `N_NODES` - num of nodes inside the cluster.    
* `IPv4_CLUSTER` - ip_address:port for each node.     
* `PGLD_DB_PATHNAME_TEMPLATE` - path template where postgres database clusters will be initialized. 
* `ENABLE_LOG` - turn on\off logging of the algorithm.
* `LOG_NAME` - the path of log relatively `PGLD_DB_PATHNAME_TEMPLATE` directory.
> The full desctiprion of config file see in [certain chapter](#config-file). 


### 4. Launching.
> There is two ways to launch.
1. Launch each node by **_nodelaunch.sh [NODE_ID]_**
2. Launch test stand.

## Test stand (Linux only)
#### Description
Test stand - `N_NODES` network namespaces with virtual ethernet interfaces. There is a bridge in root namespace, all veth interfaces connected on it. Network package control implemented by ebtables, adding rules to the FORWARD chain.
#### Usage
1. Launch `$ ./script/**_test_stand/net_init.sh`. It will create `N_NODES` network namespaces and set up the network.
2. Use `$ ./script/**_test_stand/ctl.sh` for interactive control.
    - **h, help, h, ?** - help.
    - **st, start [ALL | NODE IDs]** - pg_ctl start all (some) nodes.
    - **sp, stop [ALL | NODE IDs]** - pg_ctl stop all (some) nodes.
    - **d, disconnect ONE ANOTHER [both]**- prohibit sending packets from ONE to ANOTHER with [both] parameter prohibit sending in both directs."
    - **r, reconnect ONE ANOTHER [both]** - vice versa as disconnect.
    - **b, brainsplit FIRST_GROUP_IDs SECOND_GROUP_ID**s - split into two groups where no one node from FIRST group cannot send packets to someone in SECOND group and vice versa.
    - **statistics, stat** - network rools stat
    - **f, flush** - removes all network restrictions
    - **exit, q, quit** - exit the script
3. Use **_./script/watchpsql.sh [NODE_ID]_** NODE_ID from `0` to `N_NODES`

## Config file
#### Cluster configuration
1. Test stand configuration. 
> Initialize `IPv4_CLUSTER` as "192.168.10.10:port0, 192.168.10.11:port1, 192.168.10.12:port2, ...", use user ports (1024-49151). Test stand adds ip addresses for nodes in network (`NETWORK` parameter) staring with .10 node. (`NETWORK` assigned as default 192.168.10)
2. Arbitrary placement. 
> Assing ip:addr for each `N_NODES` nodes. Every node will be waiting for messages from other nodes on this socket.

