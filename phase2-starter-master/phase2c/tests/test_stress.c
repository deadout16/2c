/*
 * stress tester. Creates lots of worker processes each of which write a random number of
 * sectors to the disk, sleep a while, then read back what they wrote and verify it. The
 * workers will read/write the entire disk.
 *
 */

#include <string.h>
#include <stdlib.h>
#include <usloss.h>
#include <phase1.h>
#include <phase2.h>
#include <assert.h>
#include <libuser.h>
#include <libdisk.h>

#include "tester.h"
#include "phase2Int.h"

static int passed = FALSE;

static int diskSize;    // # of sectors in a disk

#define NUM_WORKERS     20 // number of worker processes per disk
#define MAX_SECTORS     40 // max. # of sectors assigned to each worker
#define NUM_ITERATIONS  10 // # of times to read/write disk
#define MAX_SLEEP       3  // max. # of seconds to sleep between writing and reading

#define MIN(a,b) ((a) < (b) ? (a) : (b))

typedef struct Task {
    int unit;           // which disk
    int start;          // starting sector
    int sectors;        // # of sectors in range
} Task;

int Worker(void *arg) 
{
    Task *task = (Task *) arg;
    int size;
    char *output;
    char *input;
    int rc;
    int sectorSize;
    int sectors;
    int pid;

    Sys_GetPid(&pid);

    USLOSS_Console("Worker: pid %d unit %d start %d sectors %d\n",
                   pid, task->unit, task->start, task->sectors);

    rc = Sys_DiskSize(task->unit, &sectorSize, &sectors); 
    assert(rc == P1_SUCCESS);
    size = task->sectors * sectorSize;
    output = malloc(size);
    input = malloc(size);


    memset(output, (char) pid, size);

    for (int i = 0; i < NUM_ITERATIONS; i++) {
        memset(input, '\0', size);
        rc = Sys_DiskWrite(output, task->start, task->sectors, task->unit);
        TEST(rc, P1_SUCCESS);

        rc = Sys_Sleep(random() % (MAX_SLEEP + 1));
        assert(rc == P1_SUCCESS);

        rc = Sys_DiskRead(input, task->start, task->sectors, task->unit);
        TEST(rc, P1_SUCCESS);

        rc = memcmp(output, input, size);
        TEST(rc, 0);
    }
    return 17;
}

int P3_Startup(void *arg) {

    int workers = 0;
    int rc;
    int pid;
    int status;

    for (int unit = 0; unit < USLOSS_DISK_UNITS; unit++) {
        int start = 0;
        int numSectors;
        int sectorSize;
        int sectors;

        rc = Sys_DiskSize(unit, &sectorSize, &sectors); 
        assert(rc == P1_SUCCESS);
        for (int i = 0; (i < NUM_WORKERS) && (start < sectors); i++ ) {
            Task *task = (Task *) malloc(sizeof(Task));
            if (i == NUM_WORKERS-1) {
                // last worker does the rest of the disk
                numSectors = sectors - start;
            } else {
                int r = (random() % MAX_SECTORS) + 1;
                numSectors = MIN(r, sectors - start);
            }
            task->unit = unit;
            task->start = start;
            task->sectors = numSectors;
            start += numSectors;
            rc = Sys_Spawn(MakeName("worker", (unit * 100) + i), Worker, task, 
                          4*USLOSS_MIN_STACK, 3, &pid);
            assert(rc == P1_SUCCESS);
            workers++;
        }
        assert(start == sectors);
    }

    for (int i = 0; i < workers; i++) {
        rc = Sys_Wait(&pid, &status);
        assert(rc == P1_SUCCESS);
        assert(status == 17);
    }
    return 11;
}

int P2_Startup(void *arg)
{
    int rc, waitPid, status, p3Pid;

    P2ClockInit();
    P2DiskInit();
    rc = P2_Spawn("P3_Startup", P3_Startup, NULL, 4*USLOSS_MIN_STACK, 3, &p3Pid);
    TEST(rc, P1_SUCCESS);
    rc = P2_Wait(&waitPid, &status);
    TEST(rc, P1_SUCCESS);
    TEST(waitPid, p3Pid);
    TEST(status, 11);
    P2DiskShutdown();
    P2ClockShutdown();
    PASSED();
    return 0;
}

void test_setup(int argc, char **argv) {
    int rc;

    DeleteAllDisks();
    // Create the disks.
    int tracks = ((NUM_WORKERS * MAX_SECTORS / 2 - 1) / USLOSS_DISK_TRACK_SIZE) + 1;
    for (int i = 0; i < USLOSS_DISK_UNITS; i++) {
        rc = Disk_Create(NULL, i, tracks);
        assert(rc == 0);
    }
    diskSize = tracks * USLOSS_DISK_TRACK_SIZE;
}

void test_cleanup(int argc, char **argv) {
    DeleteAllDisks();
    if (passed) {
        USLOSS_Console("You passed all the tests! Treat yourself to a cookie!\n");
        USLOSS_Console("TEST PASSED.\n");
    }
}
void finish(int argc, char **argv) {}
