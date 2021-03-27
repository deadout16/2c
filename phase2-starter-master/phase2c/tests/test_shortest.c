/*
 * Tests that the disk driver does shortest seek first. The Controller creates several workers
 * that call P2_DiskWrite to write NUMSECTORS sectors of data starting at the sectors specified
 * in the "firsts" array. The first of these will call P2_DiskWrite with starting sector 0, which
 * will cause the disk driver to perform the request. While it is blocked the remaining workers
 * will submit their requests so that when the disk driver finishes the first request it will
 * have several requests from which to choose. It should perform the requests in the order in the
 * "order" array. Note that one of the workers submits a second request after its first finishes, 
 * to test that the disk driver notices new requests and adds them to its pool of requests.
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

#define NUMSECTORS 20
#define UNIT 0
#define TRACKS 100

#define LOCK(lid) { \
    int _rc = P1_Lock(lid); \
    assert(_rc == P1_SUCCESS); \
}

#define UNLOCK(lid) { \
    int _rc = P1_Unlock(lid); \
    assert(_rc == P1_SUCCESS); \
}

static int order[100]; // order in which the requests were processed.
static int finished = 0;       // # of finished requests
static int lock;                // lock for above variables

int Worker(void *arg) 
{
    int first = (int) arg;
    char *buffer = malloc(NUMSECTORS * USLOSS_DISK_SECTOR_SIZE);
    memset(buffer, 0xAD, NUMSECTORS * USLOSS_DISK_SECTOR_SIZE);

    while(1) {
        int rc = P2_DiskWrite(UNIT, first, NUMSECTORS, buffer);
        TEST_RC(rc, P1_SUCCESS);

        LOCK(lock);
        order[finished++] = first;
        UNLOCK(lock);

        // have one of the workers add a new request.
        if (first == 680) {
            first = 880;
        } else {
            break;
        }        
    }
    return 50;
}

static int firsts[] = {0,1345,115,680,950,615};
static int numWorkers = sizeof(firsts) / sizeof(int);
static int expected[] = {0,115,615,680,950,880,1345};

int Controller(void *arg) {

    int rc;
    int pid;
    int status;


    for (int i = 0; i < numWorkers; i++) {
        rc = P1_Fork(MakeName("Worker", i), Worker, (void *) firsts[i], 
                          4*USLOSS_MIN_STACK, 3, &pid);
            TEST_RC(rc, P1_SUCCESS);
    }
    for (int i = 0; i < numWorkers; i++) {
        rc = P1_Join(&pid, &status);
        TEST_RC(rc, P1_SUCCESS);
        TEST(status, 50);
    }

    // verify that the requests completed in the correct order

    TEST(finished, sizeof(firsts) / sizeof(int) + 1);
    for (int i = 0; i < finished; i++) {
        TEST(order[i], expected[i]);
    }
    passed = TRUE;
    return 11;
}

int P2_Startup(void *arg)
{
    int rc, pid, status;

    P2ClockInit();
    P2DiskInit();
    rc = P1_LockCreate("Worker Lock", &lock);
    TEST_RC(rc, P1_SUCCESS);
    rc = P1_Fork("Controller", Controller, NULL, 4*USLOSS_MIN_STACK, 4, &pid);
    TEST(rc, P1_SUCCESS);
    rc = P1_Join(&pid, &status);
    TEST(rc, P1_SUCCESS);
    TEST(status, 11);
    P2DiskShutdown();
    P2ClockShutdown();
    return 0;
}

void test_setup(int argc, char **argv) {
    int rc;

    DeleteAllDisks();
    rc = Disk_Create(NULL, UNIT, TRACKS);
    assert(rc == 0);
}

void test_cleanup(int argc, char **argv) {
    DeleteAllDisks();
    if (passed) {
        PASSED();
    }
}
void finish(int argc, char **argv) {}
