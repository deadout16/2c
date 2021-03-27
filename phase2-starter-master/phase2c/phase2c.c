#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

#include <usloss.h>
#include <phase1.h>

#include "phase2Int.h"


static int      DiskDriver(void *);
static void     ReadStub(USLOSS_Sysargs *sysargs);
static void     WriteStub(USLOSS_Sysargs *sysargs);
static void     SizeStub(USLOSS_Sysargs *sysargs);

static char *
MakeName(char *prefix, int suffix)
{
    static char name[P1_MAXNAME];
    snprintf(name, sizeof(name), "%s%d", prefix, suffix);
    return name;
}

/*
 * P2DiskInit
 *
 * Initialize the disk data structures and fork the disk drivers.
 */
void 
P2DiskInit(void) 
{
    int rc;

    // initialize data structures here including lock and condition variables

    rc = P2_SetSyscallHandler(SYS_DISKREAD, ReadStub);
    assert(rc == P1_SUCCESS);

    rc = P2_SetSyscallHandler(SYS_DISKWRITE, WriteStub);
    assert(rc == P1_SUCCESS);

    rc = P2_SetSyscallHandler(SYS_DISKSIZE, SizeStub);
    assert(rc == P1_SUCCESS);

    for (int unit = 0; unit < USLOSS_DISK_UNITS; unit++) {
        int pid;
        rc = P1_Fork(MakeName("Disk Driver ", unit), DiskDriver, (void *) unit, USLOSS_MIN_STACK*4, 
                     1, &pid);
        assert(rc == P1_SUCCESS);
    }
}

/*
 * P2DiskShutdown
 *
 * Stop the disk drivers.
 */

void 
P2DiskShutdown(void) 
{
}

/*
 * DiskDriver
 *
 * Kernel process that manages a disk device and services disk I/O requests from other processes.
 * Note that it may require several disk operations to service a single I/O request. A disk
 * operation is performed by sending a request of type USLOSS_DeviceRequest to the disk via
 * USLOSS_DeviceOutput, then waiting for the operation to finish via P1_WaitDevice. The status
 * returned by P1_WaitDevice will tell you if the operation was successful or not.
 */
static int 
DiskDriver(void *arg) 
{
    int unit = (int) arg;
    /****
    repeat
        choose request with shortest seek from current track
        seek to proper track if necessary
        while request isn't complete
             for all sectors to be read/written in current track
                read/write sector
             seek to next track
        wake the waiting process
    until P2DiskShutdown has been called
    ****/
    USLOSS_Console("DiskDriver PID %d unit %d exiting.\n", P1_GetPid(), unit);
    return 0;
}

/*
 * P2_DiskRead
 *
 * Reads the specified number of sectors from the disk starting at the first sector.
 */
int 
P2_DiskRead(int unit, int first, int sectors, void *buffer) 
{
    // validate parameters
    // give request to the unit's device driver
    // wait until device driver completes the request
    return P1_SUCCESS;
}

/*
 * P2_DiskWrite
 *
 * Writes the specified number of sectors to the disk starting at the first sector.
 */
int 
P2_DiskWrite(int unit, int first, int sectors, void *buffer) 
{
    // validate parameters
    // give request to the unit's device driver
    // wait until device driver completes the request
    return P1_SUCCESS;
}

/*
 * P2_DiskSize
 *
 * Returns the size of the disk.
 */
int 
P2_DiskSize(int unit, int *sector, int *disk) 
{

    // validate parameter
    // give request to the unit's device driver
    // wait until device driver completes the request
    return P1_SUCCESS;
}

static void 
ReadStub(USLOSS_Sysargs *sysargs) 
{
    int     rc;
    rc = P2_DiskRead((int) sysargs->arg4, (int) sysargs->arg3, (int) sysargs->arg2, sysargs->arg1);
    sysargs->arg4 = (void *) rc;
}

static void 
WriteStub(USLOSS_Sysargs *sysargs) 
{
    int     rc;
    rc = P2_DiskWrite((int) sysargs->arg4, (int) sysargs->arg3, (int) sysargs->arg2, sysargs->arg1);
    sysargs->arg4 = (void *) rc;
}


static void 
SizeStub(USLOSS_Sysargs *sysargs) 
{
    int     rc;
    int     sector;
    int     disk;

    rc = P2_DiskSize((int) sysargs->arg1, &sector, &disk);
    sysargs->arg1 = (void *) sector;
    sysargs->arg2 = (void *) disk;
    sysargs->arg4 = (void *) rc;
}


