/*
 * p3stubs.c
 *
 * These are stub implementations of P3_AllocatePageTable and P3_FreePageTable
 * for use in phases 1 and 2 of the project.
 */

#include <stdio.h>
#include <string.h>
#include "phase1.h"
#include "assert.h"

#define myassert(e) \
    ((void) ((e) ? ((void)0) : __myassert (#e, __FILE__, __LINE__)))

#define __myassert(e, file, line) \
    (p3mode ? __assert(e, file, line) : p3aborts++)

static int allocated[P1_MAXPROC];
static int initialized = 0;

int p3mode = 1;
int p3aborts = 0;

USLOSS_PTE *
P3_AllocatePageTable(int pid)
{
    if (! initialized) {
        memset(allocated, 0, sizeof(allocated));
        initialized = 1;
    }
    myassert((pid >= 0) && (pid < P1_MAXPROC));
    myassert(allocated[pid] == 0);
    allocated[pid] = 1;
    return NULL;
}

void
P3_FreePageTable(int pid)
{
    myassert(initialized);
    myassert((pid >= 0) && (pid < P1_MAXPROC));
    myassert(allocated[pid] == 1);
    allocated[pid] = 0;
}

int
P3_VmInit(int mappings, int pages, int frames, int pagers2)
{
    return 0;
}

void
P3_VmShutdown(void)
{
    return;
}


