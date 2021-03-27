
/*
 * test_sleep.c
 *
 * Creates NUM_SLEEPERS children that sleep for 0-9 seconds. Each sleeper uses the USLOSS clock
 * to verify that it slept for at least the specified number of seconds. Should take about 9 seconds
 * to run unless you specify -R on the command line so it uses virtual time in which case it will
 * finish quite quickly. 
 *
 */


#include <assert.h>
#include <usloss.h>
#include <stdlib.h>
#include <stdarg.h>
#include <libuser.h>
#include <sys/time.h>

#include "tester.h"
#include "phase2Int.h"

#define NUM_SLEEPERS 40

/*
 * Sleeper
 *
 * Sleeps for the number of seconds in arg and checks that it slept long enough.
 *
 */

int Sleeper(void *arg) {
    int start, end, rc;
    int seconds = (int) arg;
    Sys_GetTimeOfDay(&start);
    rc = Sys_Sleep(seconds);
    TEST_RC(rc, P1_SUCCESS);
    Sys_GetTimeOfDay(&end);
    int ms = seconds * 1000;
    int duration = (end - start) / 1000; // duration in ms
    // should be no more than 200ms longer than the specified duration and not less
    TEST((duration >= ms) && (duration <= ms+200), 1);
    return 0;
}

/*
 * P3_Startup
 *
 * Creates NUM_SLEEPERS children who sleep for random amounts of time.
 *
 */
int
P3_Startup(void *arg)
{
    int status, rc;
    int pid = -1;

    for (int i = 0; i < NUM_SLEEPERS; i++) {
        int duration = random() % 10;
        rc = Sys_Spawn(MakeName("Sleeper", i), Sleeper, (void *) duration, USLOSS_MIN_STACK, 5, &pid);
        TEST_RC(rc, P1_SUCCESS);
    }

    for (int i = 0; i < NUM_SLEEPERS; i++) {
        rc = Sys_Wait(&pid, &status);
        TEST_RC(rc, P1_SUCCESS);
    }
    return 11;
}

int P2_Startup(void *arg)
{
    int rc, waitPid = -1, status = 0, p3Pid = -2;
    struct timeval t;

    gettimeofday(&t, NULL);
    srandom(t.tv_sec);
    P2ClockInit();
    rc = P2_Spawn("P3_Startup", P3_Startup, NULL, 4*USLOSS_MIN_STACK, 2, &p3Pid);
    TEST_RC(rc, P1_SUCCESS);

    rc = P2_Wait(&waitPid, &status);
    TEST_RC(rc, P1_SUCCESS);
    TEST(waitPid, p3Pid);
    TEST(status, 11);
    P2ClockShutdown();
    PASSED();
    return 0;
}

void test_setup(int argc, char **argv) {
    // Do nothing.
}

void test_cleanup(int argc, char **argv) {}

void finish(int argc, char **argv) {}
