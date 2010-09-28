#pragma once

#include <common/generic.h>
#include <interrupts/interface.h>
#include <memory/paging/paging.h>

typedef struct {
	uint32 pid; // process id (>= 1)
	uint32 parent; // process id of parent (or zero if none)
	char name[100]; // a human-readable name
	
	// information needed to continue the process
	
	registers_t regs; // saves state information, err_code and int_num have no importance of course
	// regs can conveniently be read as the stack: set esp = &regs and you can start with pop'ing of ds
	
	// virtual address space
	pageDirectory_t* pageDirectory;
} process_t;


// temporary function
void createProcess(char name[100], void function());
