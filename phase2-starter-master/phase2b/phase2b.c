#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <usloss.h>
#include <phase1.h>

#include "phase2Int.h"


static int      ClockDriver(void *);
static void     SleepStub(USLOSS_Sysargs *sysargs);

static int      now; // current time

/*
 * P2ClockInit
 *
 * Initialize the clock data structures and fork the clock driver.
 */
void 
P2ClockInit(void) 
{
    int rc;

    P2ProcInit();

    // initialize data structures here

    rc = P2_SetSyscallHandler(SYS_SLEEP, SleepStub);
    assert(rc == P1_SUCCESS);

    // fork the clock driver here
}

/*
 * P2ClockShutdown
 *
 * Clean up the clock data structures and stop the clock driver.
 */

void 
P2ClockShutdown(void) 
{
    // stop clock driver
}

/*
 * ClockDriver
 *
 * Kernel process that manages the clock device and wakes sleeping processes.
 */
static int 
ClockDriver(void *arg) 
{

    while(1) {
        int rc;

        // wait for the next interrupt
        rc = P1_DeviceWait(USLOSS_CLOCK_DEV, 0, &now);
        if (rc == P1_WAIT_ABORTED) {
            break;
        }
        assert(rc == P1_SUCCESS);

        // wakeup any sleeping processes whose wakeup time has arrived
    }
    return P1_SUCCESS;
}

/*
 * P2_Sleep
 *
 * Causes the current process to sleep for the specified number of seconds.
 */
int 
P2_Sleep(int seconds) 
{
    // update current time and determine wakeup time
    // add current process to data structure of sleepers
    // wait until it's wakeup time
    return P1_SUCCESS;
}

/*
 * SleepStub
 *
 * Stub for the Sys_Sleep system call.
 */
static void 
SleepStub(USLOSS_Sysargs *sysargs) 
{
    int seconds = (int) sysargs->arg1;
    int rc = P2_Sleep(seconds);
    sysargs->arg4 = (void *) rc;
}

