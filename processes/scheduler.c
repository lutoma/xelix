#include <processes/scheduler.h>
#include <common/generic.h>


process_t* currentProcess = 0;

uint32 currentProcessIndex = 0;

process_t* processes[7] = {0, 0, 0, 0, 0, 0, 0}; // TODO: replace by linked list or something else with more arbitrary length




/*
 * called from the assembler switchcontext.asm
 * * gets the esp with status stuff pushed as the parameter -> should be saved
 * * has to return the esp of the next process which should be run -> has to change virtual memory space
 * returns 0 if multiprocessing is not enabled yet
 */
uint32* schedule(uint32* esp)
{
	if(currentProcess == 0)
	{
		return 0;
	}
	currentProcess->esp = esp;
	
	currentProcessIndex++;
	if(processes[currentProcessIndex] == 0)
	{
		currentProcessIndex = 0;
	}
	
	currentProcess = processes[currentProcessIndex];
	
	// TODO change virtual memory space
	
	return currentProcess->esp;
}

void schedule_addProcess(process_t* process)
{
	int i = 0;
	while(processes[i] != 0)
		i++;
	processes[i] = process;
}



