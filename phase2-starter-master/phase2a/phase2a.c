#include <stdlib.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <stdio.h>
#include <assert.h>
#include <libuser.h>
#include <usyscall.h>

#include "phase2Int.h"

#define TAG_KERNEL 0
#define TAG_USER 1

static void SpawnStub(USLOSS_Sysargs *sysargs);

/*
 * IllegalHandler
 *
 * Handler for illegal instruction interrupts.
 *
 */

static void 
IllegalHandler(int type, void *arg) 
{
    // do something here
}

/*
 * SyscallHandler
 *
 * Handler for system call interrupts.
 *
 */

static void 
SyscallHandler(int type, void *arg) 
{
    USLOSS_Sysargs *sysargs = (USLOSS_Sysargs *) arg;
    // call the proper handler for the system call.
    sysargs->arg4 = (void *) P2_INVALID_SYSCALL;
}


/*
 * P2ProcInit
 *
 * Initialize everything.
 *
 */

void
P2ProcInit(void) 
{
    int rc;

    USLOSS_IntVec[USLOSS_ILLEGAL_INT] = IllegalHandler;
    USLOSS_IntVec[USLOSS_SYSCALL_INT] = SyscallHandler;

    // call P2_SetSyscallHandler to set handlers for all system calls
    rc = P2_SetSyscallHandler(SYS_SPAWN, SpawnStub);
    assert(rc == P1_SUCCESS);
}

/*
 * P2_SetSyscallHandler
 *
 * Set the system call handler for the specified system call.
 *
 */

int
P2_SetSyscallHandler(unsigned int number, void (*handler)(USLOSS_Sysargs *args))
{
    return P1_SUCCESS;
}

/*
 * P2_Spawn
 *
 * Spawn a user-level process.
 *
 */
int 
P2_Spawn(char *name, int(*func)(void *arg), void *arg, int stackSize, int priority, int *pid) 
{
    // do something here
    return P1_SUCCESS;
}

/*
 * P2_Wait
 *
 * Wait for a user-level process.
 *
 */

int 
P2_Wait(int *pid, int *status) 
{
    // do something here
    return P1_SUCCESS;
}

/*
 * P2_Terminate
 *
 * Terminate a user-level process.
 *
 */

int 
P2_Terminate(int status) 
{
    // do something here
    return P1_SUCCESS;

}

/*
 * SpawnStub
 *
 * Stub for Sys_Spawn system call. 
 *
 */

static void 
SpawnStub(USLOSS_Sysargs *sysargs) 
{
    int (*func)(void *) = sysargs->arg1;
    void *arg = sysargs->arg2;
    int stackSize = (int) sysargs->arg3;
    int priority = (int) sysargs->arg4;
    char *name = sysargs->arg5;
    int pid;
    int rc = P2_Spawn(name, func, arg, stackSize, priority, &pid);
    if (rc == P1_SUCCESS) {
        sysargs->arg1 = (void *) pid;
    }
    sysargs->arg4 = (void *) rc;
}
