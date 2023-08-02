# pg_leader
PostgreSQL extension. RAFT-based algorithm realization of consensus algorithm of choosing the leader.

## Requirement
- Linux >2.6 (for ebtables)
- Installed Postgres (and export PATH=/path/to/postgres/bin:$PATH)

## Contents
- [/config](./config/pg_leader.config) contains the configuration file of pg_leader extension.

- [/script](./script) contains script are using for launching node with pg_leader extension, and one more for observing it via the shell. Scripts are written on bash.
    - **_nodelaunch.sh_** for launching the node (postgres+pg_leader)
    - **_watchpsql.sh_** for observing node condition: it **node ID**, **term** and **leader ID** on current node (leader ID may probably be different on nodes, the aim of this extension that each node has the same leader ID)
    - **_test_stand/net_init.sh_** are using for initialization network namespaces and network between them. 
    - **_test_stand/ctl.sh_** are using for interactive control of the test stand.

## Installation
    Launching nodes, cluster observing and launching test stand may be carried out using scripts from /script directory.

##### 0. Clone the repository to the arbitrary directory 
``$ git clone https://gitpglab.postgrespro.ru/t.shalnev/pg_leader.git``
##### 1. [Config file](./config/pg_leader.config) editing.
`N_NODES`
> To start cluster you should edit params `N_NODES` and `IPv4_CLUSTER` with cluster nodes description.  
Postgres database clusters will be installed in `PGLD_DB_PATHNAME_PREFIX`. Internal algorithm logger turned on/off by `ENABLE_LOG`. See **"Config file"** for more information.
##### 2. Building.
> Start Makefile in pg_leader directory `$ make install`.    Now we have the extension installed above Postgres.
##### 4. Launching.
> There is two ways to launch.
1. Launch each node by **_nodelaunch.sh [NODE_ID]_**
2. Launch test stand.
### Test stand (Linux only)
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

