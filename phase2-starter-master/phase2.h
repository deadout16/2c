/*
 * These are the definitions for Phase 2 of the project.
 * DO NOT MODIFY THIS FILE.
 *
 */

#ifndef _PHASE2_H
#define _PHASE2_H

#include <usyscall.h>

/* 
 * Function prototypes for this phase.
 */

#ifndef CHECKRETURN
#define CHECKRETURN __attribute__((warn_unused_result))
#endif

extern  int	    P2_Sleep(int seconds) CHECKRETURN;


extern  int     P2_DiskRead(int unit, int first, int sectors, void *buffer) CHECKRETURN;
extern  int	    P2_DiskWrite(int unit, int first, int sectors, void *buffer) CHECKRETURN;
extern  int 	P2_DiskSize(int unit, int *sector, int *disk) CHECKRETURN;

extern  int     P2_Spawn(char *name, int (*func)(void *arg), void *arg, int stackSize, 
                         int priority, int *pid) CHECKRETURN;
extern  int     P2_Wait(int *pid, int *status) CHECKRETURN;
extern  int     P2_Terminate(int status);
extern  int     P2_SetSyscallHandler(unsigned int number, 
                        void (*handler)(USLOSS_Sysargs *args)) CHECKRETURN;

extern	int 	P3_Startup(void *) CHECKRETURN;



/*
 * Phase 2 specific error codes
 */


#define P2_INVALID_SYSCALL      -25
#define P2_INVALID_SECONDS      -26
#define P2_INVALID_FIRST        -27
#define P2_INVALID_SECTORS      -28
#define P2_NULL_ADDRESS         -29
#define P2_NOT_SPAWNED          -30

#endif


