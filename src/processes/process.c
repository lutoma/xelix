// Some nice-to-have functions for easier process handling.
#include <processes/process.h>

#include <memory/kmalloc.h>
#include <common/log.h>

// Start process. The name parameter is here for future use.
void process_create(char name[100], void function())
{
	log("process: Spawned new process with name %s\n", name);
	(*function) (); // Run process
	return;
}
