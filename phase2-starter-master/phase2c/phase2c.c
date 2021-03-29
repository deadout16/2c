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

typedef struct Pool{
    int first; // values between 0-15
    int sectors; // the amount of secors to read/write
    int unit; // unit of disk driver (0 or 1)
    int track; // calculated when being created
    USLOSS_DeviceRequest *task;
    void *buffer; // What will be filled by disk
    int condId; // condition variable this task is waiting on
} Pool;

Pool *pools[P1_MAXPROC];
int currentTrack[2];

int lockId;
// state variable
int poolSize;

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
    int i;

    // initialize data structures here including lock and condition variables
    rc = P1_LockCreate("lock", &lockId);
    assert(rc == P1_SUCCESS);
    poolSize = 0;

    for(i = 0; i < P1_MAXPROC; i++){
        pools[i] = NULL;
    }

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
    currentTrack[0] = 0;
    currentTrack[1] = 0;
}

/*
 * P2DiskShutdown
 *
 * Stop the disk drivers.
 */

void 
P2DiskShutdown(void) 
{
    P1_DeviceAbort(USLOSS_DISK_DEV, 1);
    P1_DeviceAbort(USLOSS_DISK_DEV, 2);
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

 // TODO: What is he talking bout with P1_WaitDevice()
static int 
DiskDriver(void *arg) 
{
    int unit = (int) arg;
    int status;
    int rc;
    int shortestIndex; // index of pools with the shortest job
    int shortestDistance
    int i;
    int sectorsTouched = 0; // represent the sectors touched while looping though
    int start;
    Pool *currentTask;
    USLOSS_DeviceRequest *req;
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
    while(1){
        // TODO: I dont know how to start this while its waiting for an interrupt
        if(poolSize == 0){
            rc = P1_DeviceWait(USLOSS_DISK_DEV, unit, &status);
            assert(status == USLOSS_DEV_READY);
            if (rc == P1_WAIT_ABORTED) {
                break;
            }
        }
        P1_Lock(lockId);
        // finds the shortest distance for the head to move
        for(i = 0; i < P1_MAXPROC; i++){
            // If there is nothing there go on to the next one
            if(pools[i] == NULL){
                continue;
            }
            // Checks if proper unit and the current track is the same being used
            if(pools[i]->unit == unit && pools[i]->track == currentTrack[unit]){
                shortestIndex = i;
                break;
            }
            // Checks to see if the instruction is a size instruction
            if(pools[i]->unit == unit && pools[i]->task->opr == USLOSS_DISK_TRACKS){
                shortestIndex = i;
                break;
            }
            if(pools[i]->unit == unit && abs(pools[i]->track - currentTrack[unit]) < shortestDistance){
                shortestDistance = pools[i]->track - currentTrack[unit];
                shortestIndex = i;
            }
        }
        P1_Unlock(lockId);

        // seeks proper track if necessary
        currentTask = pools[shortestIndex];
        if(currentTask->track != currentTrack[unit]){
            req->opr = USLOSS_DISK_SEEK;
            req->reg1 = currentTask->track;
            USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, req);
            // Makes sure the device has returned properly from changing track
            rc = P1_DeviceWait(USLOSS_DISK_DEV, unit, &status);
            assert(status == USLOSS_DEV_READY);
            if (rc == P1_WAIT_ABORTED) {
                break;
            }
        }
        
        // loops until all sectors asked to be visited have been
        start = currentTask->first;
        while(1){
            // check for size
            if(currentTask->task->opr == USLOSS_DISK_TRACKS){
                USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, currentTask->task);
                rc = P1_DeviceWait(USLOSS_DISK_DEV, unit, &status);
                assert(status == USLOSS_DEV_READY);
                if (rc == P1_WAIT_ABORTED) {
                    break;
                }
                break;
            }
            // goes through all sectors in a track
            for(i = start; i < 16; i++){
                // checks if all that needs to be seen has been seen
                if(sectorsTouched == currentTask->sectors){
                    break;
                }
                // writes/reads sector
                // TODO: give only 512 bytes of the buffer to read/write each time
                USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, currentTask->task);
                rc = P1_DeviceWait(USLOSS_DISK_DEV, unit, &status);
                assert(status == USLOSS_DEV_READY);
                if (rc == P1_WAIT_ABORTED) {
                    break;
                }
                // NOTE: Watch out for a fight
                currentTask->task->reg2 = currentTask->task->reg2 + (sizeof(char) * 512);
                sectorsTouched++;
            }
            // checks if seen all sectors requested
            if(sectorsTouched == currentTask->sectors){
                break;
            }
            // go to next track
            req->opr = USLOSS_DISK_SEEK;
            req->reg1 = currentTrack[unit] + 1;
            USLOSS_DeviceOutput(USLOSS_DISK_DEV, unit, req);
            // Makes sure the device has returned properly from changing track
            rc = P1_DeviceWait(USLOSS_DISK_DEV, unit, &status);
            assert(status == USLOSS_DEV_READY);
            if (rc == P1_WAIT_ABORTED) {
                break;
            }
            // update current track and set start of sectors to 0
            currentTrack[unit]++;
            start = 0;
        }
        P1_Signal(currentTask->condId);
    }
    USLOSS_Console("DiskDriver PID %d unit %d exiting.\n", P1_GetPid(), unit);
    return 0;
}

/*
 * P2_DiskRead
 *
 * Reads the specified number of sectors from the disk starting at the first sector.
 * First is the first sector that wants to be read, sectors is the amount of sectors
 * wanted to be read starting at first.
 */
int 
P2_DiskRead(int unit, int first, int sectors, void *buffer) 
{
    int index = P1_GetPid();
    int rc;
    USLOSS_DeviceRequest *req;
    // validate parameters
    // give request to the unit's device driver
    pools[index] = (Pool *)malloc(sizeof(Pool));

    req->opr = USLOSS_DISK_READ;
    req->reg1 = first;
    req->reg2 = buffer;
    pools[index]->task = req;
    pools[index]->first = first % 16;
    pools[index]->sectors = sectors;
    pools[index]->unit = unit;
    pools[index]->track = first / 16;
    pools[index]->buffer = buffer;
    rc = P1_CondCreate((toString(index)), lockId, pools[index]->condId);
    assert(rc == P1_SUCCESS);
    // wait until device driver completes the request
    P1_Wait(pools[index]->condId);
    free(pools[index])
    pools[index] = NULL;
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
    int index = P1_GetPid();
    int rc;
    USLOSS_DeviceRequest *req;
    // validate parameters
    // give request to the unit's device driver
    pools[index] = (Pool *)malloc(sizeof(Pool));

    req->opr = USLOSS_DISK_WRITE;
    req->reg1 = first;
    req->reg2 = buffer;
    pools[index]->task = req;
    pools[index]->first = first % 16;
    pools[index]->sectors = sectors;
    pools[index]->unit = unit;
    pools[index]->track = first / 16;
    pools[index]->buffer = buffer;
    rc = P1_CondCreate((toString(index)), lockId, pools[index]->condId);
    assert(rc == P1_SUCCESS);
    // wait until device driver completes the request
    P1_Wait(pools[index]->condId);
    free(pools[index])
    pools[index] = NULL;
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
    int index = P1_GetPid();
    int rc;
    USLOSS_DeviceRequest *req;
    // validate parameter
    // give request to the unit's device driver
    pools[index] = (Pool *)malloc(sizeof(Pool));

    req->opr = USLOSS_DISK_TRACKS;
    req->reg1 = disk;
    pools[index]->task = req;
    pools[index]->first = 0;
    pools[index]->sectors = 0;
    pools[index]->unit = unit;
    pools[index]->track = 0;
    pools[index]->buffer = NULL;
    rc = P1_CondCreate((toString(index)), lockId, pools[index]->condId);
    assert(rc == P1_SUCCESS);
    // wait until device driver completes the request
    P1_Wait(pools[index]->condId);
    *disk = *disk * 16;
    *sector = 512;
    free(pools[index])
    pools[index] = NULL;
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


