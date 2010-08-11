#ifndef FILESYSTEMS_MEMFS_INTERFACE_H
#define FILESYSTEMS_MEMFS_INTERFACE_H

#include <common/generic.h>
#include <filesystems/interface.h>

// Initialises the initial ramdisk. It gets passed the address of the multiboot module,
// and returns a completed filesystem node.
fsNode_t *memfs_init(uint32 location);

#endif
