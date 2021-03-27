/*
 * Creates two workers, one of which read/writes odd sectors and the other does even. 
 * First they write disk0, then copy the contents of disk0 to disk1 and verify them
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

#define TRACKS 100
#define NUMSECTORS (TRACKS * USLOSS_DISK_TRACK_SIZE)
#define DISKUNIT 0

int Worker(void *arg) {
    int id = (int) arg;
    char buffers[2][USLOSS_DISK_SECTOR_SIZE];
    int rc;
    int pid;

    Sys_GetPid(&pid);

    memset(buffers[0], 0xF0 + id, sizeof(buffers[0]));
    memset(buffers[1], 0, sizeof(buffers[1]));

    // write alternating sectors to disk 0
    for (int i = id; i < NUMSECTORS; i += 2) {
        rc = Sys_DiskWrite(buffers[0], i, 1, 0); 
        TEST(rc, P1_SUCCESS);
    }

    // verify what we wrote
    for (int i = id; i < NUMSECTORS; i += 2) {
        rc = Sys_DiskRead(buffers[1], i, 1, 0); 
        TEST(rc, P1_SUCCESS);
        TEST(memcmp(buffers[0], buffers[1], sizeof(buffers[0])), 0);
    }

    // copy alternating sectors from disk 0 to disk 1
    for (int i = id; i < NUMSECTORS; i += 2) {
        rc = Sys_DiskRead(buffers[0], i, 1, 0); 
        TEST(rc, P1_SUCCESS);
        rc = Sys_DiskWrite(buffers[0], i, 1, 1); 
        TEST(rc, P1_SUCCESS);
    }

    // compare alternating sectors from the two disks
    for (int i = id; i < NUMSECTORS; i += 2) {
        rc = Sys_DiskRead(buffers[0], i, 1, 0); 
        TEST(rc, P1_SUCCESS);
        rc = Sys_DiskRead(buffers[1], i, 1, 1); 
        TEST(rc, P1_SUCCESS);
        TEST(memcmp(buffers[0], buffers[1], sizeof(buffers[0])), 0);
    }
    passed = TRUE;
    return 11;
}

int P2_Startup(void *arg)
{
    int rc, waitPid, status, pid;

    P2ClockInit();
    P2DiskInit();
    for (int i = 0; i < 2; i++) {
        rc = P2_Spawn(MakeName("Worker", i), Worker, (void *) i, 4*USLOSS_MIN_STACK, 3, &pid);
        TEST(rc, P1_SUCCESS);
    }
    for (int i = 0; i < 2; i++) {
        rc = P2_Wait(&waitPid, &status);
        TEST(rc, P1_SUCCESS);
        TEST(status, 11);
    }
    P2DiskShutdown();
    P2ClockShutdown();
    return 0;
}

void test_setup(int argc, char **argv) {
    int rc;

    DeleteAllDisks();
    // create disks
    for (int i = 0; i < 2; i++) {
        rc = Disk_Create(NULL, i, TRACKS);
        assert(rc == 0);
    }
}

void test_cleanup(int argc, char **argv) {
    //DeleteAllDisks();
    if (passed) {
        PASSED();
    }
}
void finish(int argc, char **argv) {}
