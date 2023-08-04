# pg_leader
PostgreSQL extension. RAFT-based algorithm realization of consensus algorithm of choosing the leader.   
The job implements the algorithm of choosing the leader. It has scripts to observe which one ID is leader according to the opinion of some node (the aim that every node has the same opinion). Besides, there is a test stand allows to interactively, by writing your own scripts control the condition of each node (turned on/off) and network between them. 

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

1. `PGLD_PATH` prepare:  
 ``$ export PGLD_PATH=/path/to/cloned/directory/pg_leader/``          
2. As well you should have exported to `PATH` /bin directory of postgres:   
``$ export PATH=/path/to/postgres/bin/:$PATH`` 
    
3. To use scripts from any place do:    
 ``$ export PATH=$PGLD_PATH/script/:$PGLD_PATH/script/test_stand/:$PATH``
```shell
export PGLD_PATH=/path/to/cloned/directory/pg_leader/
export PATH=/path/to/postgres/bin/:$PATH
export PATH=$PGLD_PATH/script/:$PGLD_PATH/script/test_stand/:$PATH
```
For more comfortable use you may append these strings to your shell configuration file - for instance, `~/.bashrc` for Bash, to have these exports in each shell launch.

### 2. Building.
1. ``$ cd $PGLD_PATH``    

2. And launch `Makefile` in pg_leader directory  
``$ make install``  

**Now we have the extension installed above Postgres.**

### 3. Configuration and launching.
**[Config](#config-file) file [(./config/pg_leader.config)](./config/pg_leader.config) is ready __out of the box__ to launch 5 nodes using [test stand](#test-stand-linux-only).** 

**There are 2 ways to launch cluster:**  
1.  __Casual launch.__ You should specify `N_NODES` and `IPv4_CLUSTER` (IPv4_CLUSTER should be the same on every node). Now in arbitrary computers you can launch the node by `` $ pgld_launchnode [ID] ``, where `ID` is index in `IPv4_CLUSTER` of this node. It's possible to observe it state by ``$ pgld_watchnode [ID]``, ID is the same as for pgld_launchnode. 

2. [__Test stand__](#test-stand-linux-only). To use this, `N_NODES`=num of nodes to launch, `IPv4_CLUSTER` should contains IPs inside __192.168.10.0/24__ network, .node part of address for each should starts with .10, e.g. __'192.168.10.10:port0 192.168.10.11:port1 192.168.10.12:port2 ...'__ . Besides, `NETWORK`=192.168.10 and `MASK`=24. 

> The full desctiprion of config file see in [certain chapter](#config-file). 

## Test stand (Linux only)
#### Description
Test stand - `N_NODES` network namespaces with virtual ethernet interfaces. There is a bridge in root namespace, all veth interfaces connected on it. Network package control implemented by ebtables, adding rules to the FORWARD chain.
#### Usage
1. Launch ``$ pgld_stnd_netinit`` . It will create `N_NODES` network namespaces and set up the network.
2. Use `$ pg_stnd_ctl` for interactive control.
    - **h, help, h, ?** - help.

    - **st, start [ALL | NODE IDs]** - pg_ctl start all (some) nodes.

    - **sp, stop [ALL | NODE IDs]** - pg_ctl stop all (some) nodes.

    - **d, disconnect ONE ANOTHER [both]**- prohibit sending packets from ONE to ANOTHER with [both] parameter prohibit sending in both directs."

    - **r, reconnect ONE ANOTHER [both]** - same as disconnect but __allow__.

    - **bs, brainsplit FIRST_GROUP_IDs; SECOND_GROUP_IDs**s - split into two groups where no one node from FIRST group cannot send packets to someone in SECOND group and vice versa.

    - **bj, brainjoin FIRST_GROUP_IDs; SECOND_GROUP_IDs** - join FIRST_GROUP_IDs and SECOND_GROUP_IDs.

    - **statistics, stat** - network rools stat

    - **f, flush** - removes all network restrictions

    - **exit, q, quit** - exit the script

3. Use ``$ pgld_watchnode [NODE_ID]`` , where NODE_ID from `0` to `N_NODES`-1

## Config file
**Parameters**:
* **General.**
    * `N_NODES` - num of nodes inside the cluster.    
    * `IPv4_CLUSTER` - ip_address:port for each node.     
    * `PGLD_DB_PATHNAME_TEMPLATE` - path template where postgres database clusters will be initialized. 
    * `EXTENSION_NAME` - name of extension. Don't need to change usually.
    * `PGLD_START_PORT` - port to connect to postgres server by psql. e.g. **PGLD_START_PORT**=5000 then for node 0 - port 5000, 1 - port 5001, 2 -port 5002. 
* **Logger**
    * `ENABLE_LOG` - turn on\off logging of the algorithm.
    * `LOG_NAME` - the path of log relatively `PGLD_DB_PATHNAME_TEMPLATE` directory.
* **Algorithm**
    * `MIN_TIMEOUT` - minimal timeout in follower, candidate state.
    * `MAX_TIMEOUT` - maximal timeout in follower, candidate state.
    * `HEARTBEAT_TIMEOUT` - each **HEARTBEAT_TIMEOUT** node in leader state. will send hearbeat message.
    
* **Test stand**
    * `PGLD_NNS_PREFIX` - name of namespaces creating while pgld_stnd_netinit.
    * `VETH_ROOT_NAME_PREFIX` - veth interface name in root namespace.
    * `VETH_LOCAL_NAME_PREFIX` - veth interface name in each namespace.
    * `BRIDGE_NAME` - name of bridge creating while pgld_stnd_netinit
    * `NETWORK` - ip address assigning to veth. 
    * `MASK` - mas assigning to veth.

 
