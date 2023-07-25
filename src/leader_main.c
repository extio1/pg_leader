#include "../include/pgld.h"
#include "../include/ld_types.h"

#include "postgres.h"
#include "utils/builtins.h"
#include "fmgr.h"
#include "tcop/utility.h"

#include <stdbool.h>
#include <stddef.h>

/* --- bgworker --- */
#include "miscadmin.h"
#include "postmaster/bgworker.h"
#include "postmaster/interrupt.h"
#include "storage/ipc.h"
#include "storage/latch.h"
#include "storage/lwlock.h"
#include "storage/proc.h"
#include "storage/shmem.h"

/* these headers are used by this particular worker's code */
#include "access/xact.h"
#include "executor/spi.h"
#include "fmgr.h"
#include "lib/stringinfo.h"
#include "pgstat.h"
#include "utils/builtins.h"
#include "utils/snapmgr.h"
#include "tcop/utility.h"

PG_MODULE_MAGIC;

/* --- pg shared memory hooks --- */

static shmem_request_hook_type prev_shmem_request_hook = NULL;
static shmem_startup_hook_type prev_shmem_startup_hook = NULL;

void _PG_init(void);  

static void shmem_request(void);
static void shmem_startup(void);

struct shared_info_node* shared_info_node = NULL;

void
_PG_init(void)
{
    BackgroundWorker worker;

    if(!process_shared_preload_libraries_in_progress){
        elog(FATAL, "pg_leader - please use shared_preload_libraries=pg_leader");
    }

    // Hooks for shared memory usage

    prev_shmem_request_hook = shmem_request_hook;
    shmem_request_hook = shmem_request;

    prev_shmem_startup_hook = shmem_startup_hook;
    shmem_startup_hook = shmem_startup;

    // Backgroung worker initialization

    memset(&worker, 0, sizeof(worker));
	worker.bgw_flags = BGWORKER_SHMEM_ACCESS |
		               BGWORKER_BACKEND_DATABASE_CONNECTION;    
	worker.bgw_start_time = BgWorkerStart_RecoveryFinished;
	worker.bgw_restart_time = BGW_NEVER_RESTART;
	sprintf(worker.bgw_library_name, "pg_leader");
	sprintf(worker.bgw_function_name, "node_routine");
    sprintf(worker.bgw_name, "pg_leader node main process");
	worker.bgw_notify_pid = 0;

    RegisterBackgroundWorker(&worker);

    elog(LOG, "pg_leader proccess succesfully launched");
}

// Requesting for shared recources (memory, lock)
static void
shmem_request(void)
{
    if(prev_shmem_request_hook != NULL){
        prev_shmem_request_hook();
    }

    RequestAddinShmemSpace(sizeof(struct shared_info_node));
}


// Initialize aciquired shared recources 
static void
shmem_startup(void)
{
    bool found;

    if(prev_shmem_startup_hook != NULL){
        prev_shmem_startup_hook();
    }

    LWLockAcquire(AddinShmemInitLock, LW_EXCLUSIVE);

    shared_info_node = ShmemInitStruct("pg_leader node shared part", sizeof(struct shared_info_node),
                                   &found);
    if(!found) {
        shared_info_node->leader_id = -1;
        shared_info_node->current_node_term = 0;
    }

    LWLockRelease(AddinShmemInitLock);
}