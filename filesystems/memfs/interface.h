#ifndef FILESYSTEMS_MEMFS_INTERFACE_H
#define FILESYSTEMS_MEMFS_INTERFACE_H

#include <common/generic.h>
#include <filesystems/interface.h>

typedef struct
{
   uint32 fileCount; // The number of files in the ramdisk.
} memfsHeader_t;

typedef struct
{
   uint8 magic;     // Magic number, for error checking.
   sint8 name[64];  // Filename.
   uint32 offset;   // Offset in the initrd that the file starts.
   uint32 length;   // Length of the file.
} memfsFileHeader_t;

// Initialises the initial ramdisk. It gets passed the address of the multiboot module,
// and returns a completed filesystem node.
fsNode_t *initialiseMemfs(uint32 location);

#endif
