/*
 * test_basic.c
 *
 * Comprehensive test of basic functionality. Doesn't test everything, but tests quite a bit.
 *
 */

#include <string.h>
#include <stdlib.h>
#include <usloss.h>
#include <phase1.h>
#include <assert.h>
#include <libuser.h>

#include "tester.h"
#include "phase2Int.h"

static int childPid = -1, p2Pid = -1, p3Pid = -1;

/*
 * CheckName
 *
 * Verifies that the process with the specified pid has the specified name.
 * Tests Sys_GetProcInfo.
 */

static void CheckName(char *name, int pid) 
{
    P1_ProcInfo info;

    int rc = Sys_GetProcInfo(pid, &info);
    TEST_RC(rc, P1_SUCCESS);
    TEST(strcmp(info.name, name), 0);
}

/*
 * P2_Startup
 *
 * Entry point for this test. Creates the user-level process P3_Startup.
 * Tests P2_Spawn and P2_Wait.
 */

int P2_Startup(void *arg)
{
    int rc, waitPid = 0, status = 0;

    P2ProcInit();
    p2Pid = P1_GetPid();
    rc = P2_Spawn("P3_Startup", P3_Startup, NULL, 4*USLOSS_MIN_STACK, 2, &p3Pid);
    TEST_RC(rc, P1_SUCCESS);

    rc = P2_Wait(&waitPid, &status);
    TEST_RC(rc, P1_SUCCESS);
    TEST(waitPid, p3Pid);
    TEST(status, 2048);

    // should fail because we were not spawned.

    rc = P2_Terminate(16);
    TEST_RC(rc, P2_NOT_SPAWNED);

    PASSED();
    return 0;
}

/*
 * Child
 *
 * Checks its pid and those of its ancestors.
 * Tests Sys_GetPid.
 */

int Child(void *arg) {
    int pid;

    int rc = Sys_GetPid(&pid);
    TEST_RC(rc, P1_SUCCESS);
    TEST(pid, childPid);
    CheckName("Child", childPid);
    CheckName("P2_Startup", p2Pid);
    CheckName("P3_Startup", p3Pid);
    return (int) arg;
}

/*
 * P3_Startup
 *
 * Initial user-level process. 
 * Tests Sys_GetTimeOfDay, Sys_Spawn, Sys_Wait, and Sys_Terminate.
 */

int P3_Startup(void *arg) {
    int rc, waitPid, status;
    int start, finish;

    // verify we are in user mode.
    int mode = USLOSS_PsrGet() & USLOSS_PSR_CURRENT_MODE;
    TEST(mode, 0); // 0 is user mode

    Sys_GetTimeOfDay(&start);

    rc = Sys_Spawn("Child", Child, (void *) 42, USLOSS_MIN_STACK, 3, &childPid);
    TEST_RC(rc, P1_SUCCESS);

    rc = Sys_Wait(&waitPid, &status);
    TEST_RC(rc, P1_SUCCESS);
    TEST(status, 42);
    TEST(waitPid, childPid);

    // verify it's later than it was before
    Sys_GetTimeOfDay(&finish);
    TEST(finish > start, 1);

    // test a system call w/out a handler
    rc = Sys_Protect(0, 0);
    TEST_RC(rc, P2_INVALID_SYSCALL);

    // should not be able to call a kernel-level 
    // should cause an illegal instruction and
    // we should terminate with status 2048
    P1_Quit(5);
    // should not get here
    Sys_Terminate(11);
    // does not get here
    return 0;
}

void test_setup(int argc, char **argv) {
    // Do nothing.
}

void test_cleanup(int argc, char **argv) {
}

void finish(int argc, char **argv) {}
