#include <processes/scheduler.h>
#include <common/generic.h>


process_t* currentProcess = 0;

uint32 currentProcessIndex = 0;

process_t* processes[7] = {0, 0, 0, 0, 0, 0, 0}; // TODO: replace by linked list or something else with more arbitrary length




/*
 * called from the assembler switchcontext.asm
 * * gets the state of the last process as a parameter
 * * has to return pointer to state of next process
 * * has to change virtual memory space
 * returns 0 if multiprocessing is not enabled yet
 */
registers_t* schedule(registers_t regs)
{
	if(*processes == 0)
	{ // no process set up yet
		return 0;
	}
	
	if(currentProcess != 0) // or else this is the first time multiprocessing is enabled and schedule() is called
	{
		// we have to save the state of the last process
		currentProcess->regs = regs;
	}
	
	currentProcessIndex++;
	if(processes[currentProcessIndex] == 0)
	{
		currentProcessIndex = 0;
	}
	
	currentProcess = processes[currentProcessIndex];
	
	
	paging_switchPageDirectory(currentProcess->pageDirectory);
	
	regs = currentProcess->regs;
	return &regs;
}

void scheduler_addProcess(process_t* process)
{
	int i = 0;
	while(processes[i] != 0)
		i++;
	processes[i] = process;
}



