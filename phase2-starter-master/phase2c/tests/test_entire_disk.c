/*
 * test reading/writing entire disk
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
#include <fcntl.h>
#include <sys/errno.h>

#include "tester.h"
#include "phase2Int.h"

static int passed = FALSE;

#define TRACKS 100
#define DISKSIZE (TRACKS * USLOSS_DISK_TRACK_SIZE * USLOSS_DISK_SECTOR_SIZE)
#define DISKUNIT 0

static char *inBuffer;
static char *outBuffer;

int P3_Startup(void *arg) {
    inBuffer = malloc(DISKSIZE);
    memset(inBuffer, 0xFF, DISKSIZE);
    outBuffer = malloc(DISKSIZE);

    int n = 0;
    for (int i = 0; i < TRACKS; i++) {
        for (int j = 0; j < USLOSS_DISK_TRACK_SIZE; j++) {
            outBuffer[n++] = (i << 4) | j;
        }
    }
    // write the entire disk
    int rc = Sys_DiskWrite(outBuffer, 0, TRACKS * USLOSS_DISK_TRACK_SIZE, DISKUNIT); 
    TEST(rc, P1_SUCCESS);

    // read the entire disk
    rc = Sys_DiskRead(inBuffer, 0, TRACKS * USLOSS_DISK_TRACK_SIZE, DISKUNIT); 
    TEST(rc, P1_SUCCESS);

    // compare what we read with what we wrote
    TEST(memcmp(inBuffer, outBuffer, DISKSIZE), 0);
    passed = TRUE;
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
    return 0;
}

void test_setup(int argc, char **argv) {
    int rc;

    DeleteAllDisks();
    // create disk
    rc = Disk_Create(NULL, DISKUNIT, TRACKS);
    assert(rc == 0);
}

void test_cleanup(int argc, char **argv) {
    // Verify that the disk file contains what we wrote.
    int fd = OpenDisk(DISKUNIT);
    if (fd < 0) {
        perror("Unable to open disk file");
        exit(1);
    }
    int n = read(fd, inBuffer, DISKSIZE);
    assert(n == DISKSIZE);
    close(fd);
    if (memcmp(inBuffer, outBuffer, DISKSIZE) != 0) {
        USLOSS_Console("Disk file corrupted.\n");
        passed = FALSE;
    } else {
        DeleteAllDisks();
    }
    if (passed) {
        PASSED();
    }
}
void finish(int argc, char **argv) {}
