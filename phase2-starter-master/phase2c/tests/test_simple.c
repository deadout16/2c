/*
 * Tests writing one sector to the disk and reading it back. Written by a former student who
 * evidently liked cookies.
 *
 *  Created on: Mar 8, 2015
 *      Author: jeremy
 */

#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <usloss.h>
#include <phase1.h>
#include <assert.h>
#include <libuser.h>
#include <libdisk.h>

#include "tester.h"
#include "phase2Int.h"

#define TRACKS 10
#define FIRST 0

#define MSG "This is a test."

static int passed = FALSE;

int P3_Startup(void *arg) {
    char buffer[USLOSS_DISK_SECTOR_SIZE];
    strncpy(buffer, MSG, sizeof(buffer));

    USLOSS_Console("Write to the disk.\n");
    int rc = Sys_DiskWrite(buffer, FIRST, 1, 0);
    USLOSS_Console("Verify that the disk write was successful.\n");
    assert(rc == P1_SUCCESS);
    USLOSS_Console("Wrote \"%s\".\n", buffer);

    bzero(buffer, sizeof(buffer));
    USLOSS_Console("Read from the disk.\n");
    rc = Sys_DiskRead(buffer, FIRST, 1, 0);
    USLOSS_Console("Verify that the disk read was successful.\n");
    assert(rc == P1_SUCCESS);
    USLOSS_Console("Read \"%s\".\n", buffer);
    TEST(strcmp(MSG, buffer), 0);
    return 11;
}
int P2_Startup(void *arg)
{
    int rc, waitPid = -1, status = 0, p3Pid = -2;

    P2ClockInit();
    P2DiskInit();
    rc = P2_Spawn("P3_Startup", P3_Startup, NULL, 4*USLOSS_MIN_STACK, 2, &p3Pid);
    TEST(rc, P1_SUCCESS);

    rc = P2_Wait(&waitPid, &status);
    TEST(rc, P1_SUCCESS);
    TEST(waitPid, p3Pid);
    TEST(status, 11);
    P2DiskShutdown();
    P2ClockShutdown();
    USLOSS_Console("You passed all the tests! Treat yourself to a cookie!\n");
    passed = TRUE;
    return 0;
}


void test_setup(int argc, char **argv) {
    int rc;

    rc = Disk_Create(NULL, 0, TRACKS);
    assert(rc == 0);
}

void test_cleanup(int argc, char **argv) {
    char buffer[USLOSS_DISK_SECTOR_SIZE];
    // Verify the sector was written to the correct location on the disk.
    int fd = OpenDisk(0);
    if (fd < 0) {
        perror("Unable to open disk file");
        exit(1);
    }
    int n = lseek(fd, FIRST * USLOSS_DISK_SECTOR_SIZE, SEEK_SET);
    assert(n == FIRST * USLOSS_DISK_SECTOR_SIZE);
    n = read(fd, buffer, sizeof(buffer));
    assert(n == sizeof(buffer));
    close(fd);
    if (strcmp(MSG, buffer) != 0) {
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
