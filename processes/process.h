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
	
	register_t regs; // stores the register information from this process' last run. For convenience, we use the same format like in interrupts. (err_code and int_num are of course not necessary, it just makes stuff simpler because we can use the same interrupt handlers etc)
	
	uint32* esp; // the stack pointer // stack is setup as if in an interrupt, ie. *(esp+1) = eip, *(esp+2) = cs, *(esp+3) = eflags.    so iret will continue execution of this thread, if regs is properly loaded into registers and the stack is set up correctly with esp;
	
	// virtual address space
	pageDirectory_t* pageDirectory;
} process_t;





#endif
