
#include <string.h>
#include <stdlib.h>
#include <usloss.h>
#include <phase1.h>
#include <assert.h>
#include <libuser.h>
#include <libdisk.h>

#include "phase2Int.h"

static void     CreateStub(USLOSS_Sysargs *sysargs);

/*
 * I left this useful function here for you to use for debugging. If you add -DDEBUG to CFLAGS
 * this will produce output, otherwise it won't. The message is printed including the function
 * and line from which it was called. I don't remember why it is called "debug2".
 */

static void debug2(char *fmt, ...)
{
    #ifdef DEBUG
    va_list ap;
    va_start(ap, fmt);
    USLOSS_Console("[%s:%d] ", __PRETTY_FUNCTION__, __LINE__);   \
    USLOSS_VConsole(fmt, ap);
    #endif
}

int P2_Startup(void *arg)
{
    int rc, pid;

    // initialize clock and disk drivers

    debug2("starting\n");
    rc = P2_SetSyscallHandler(SYS_LOCKCREATE, CreateStub);
    assert(rc == P1_SUCCESS);
    // ...
    rc = P2_Spawn("P3_Startup", P3_Startup, NULL, 4*USLOSS_MIN_STACK, 2, &pid);
    assert(rc == P1_SUCCESS);
    
    // wait for P2_Spawn to terminate

    // shut down clock and disk drivers

    return 0;
}

static void
CreateStub(USLOSS_Sysargs *sysargs)
{
    sysargs->arg4 = (void *) P1_LockCreate((char *) sysargs->arg1, (void *) &sysargs->arg1);
}

