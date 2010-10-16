// This file provices abstraction from the filesystem drivers for normal use.

#include <common/log.h>
#include <filesystems/interface.h>
#include <memory/kmalloc.h>




// Initialize the filesystem abstraction system
void fs_init()
{

	log("Initialized filesystem abstraction\n");
}
