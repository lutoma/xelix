// Some nice-to-have functions for easier process handling.
#include <processes/process.h>

#include <memory/kmalloc.h>
#include <common/log.h>

void createProcess(char name[100], void function())
{
	log("process: Spawned new process with name %s\n", name);
	(*function) (); // Fake-run process
	return;
}
