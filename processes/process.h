#ifndef PROCESSES_PROCESS_H
#define PROCESSES_PROCESS_H

#include <common/generic.h>
#include <interrupts/interface.h>
#include <memory/paging/paging.h>

typedef struct {
	uint32 pid; // process id (>= 1)
	uint32 parent; // process id of parent (or zero if none)
	char name[100]; // a human-readable name
	
	// information needed to continue the process
	
	uint32* esp; // the stack pointer // stack is setup as if in an interrupt with (n ints of) status stuff pushed, ie. *(esp+n+1) = eip, *(esp+n+2) = cs, *(esp+n+3) = eflags. (later in user mode there will be more)   so after setting up the correct esp and popping the status stuff, iret will continue execution of this thread // see switchcontext.asm for the order of this status stuff.
	
	// virtual address space
	pageDirectory_t* pageDirectory;
} process_t;


// temporary function
void createProcess(char name[100], void function());



#endif
