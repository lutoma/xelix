#include <filesystems/memfs/interface.h>
#include <common/string.h>
#include <memory/kmalloc.h>

typedef struct
{
	uint32 fileCount; // The number of files in the ramdisk.
} memfsHeader_t;

typedef struct
{
	uint8 magic;   // Magic number, for error checking.
	char name[64];// Filename.
	uint32 offset; // Offset in the initrd that the file starts.
	uint32 length; // Length of the file.
} memfsFileHeader_t;


memfsHeader_t *memfsHeader; // The header.
memfsFileHeader_t *memfsHeaders; // The list of file headers.
struct dirent dirent;

// Read single file
static uint32 memfs_read(fsNode_t *node, uint32 offset, uint32 size, uint8 *buffer)
{
/*
	memfsFileHeader_t header = memfsHeaders[node->inode];
	if (offset > header.length)
		return 0;
	if (offset+size > header.length)
		size = header.length-offset;
	memcpy(buffer, (uint8*) (header.offset+offset), size);
	return size;
*/
}

// Read directory [aka get content]
static struct dirent *memfs_readdir(fsNode_t *node, uint32 index)
{
/*	if (index-1 >= rootNodeCount)
		return 0;
	strcpy(dirent.name, rootNodes[index-1]->name);
	dirent.name[strlen(rootNodes[index-1]->name)] = 0; // Make sure the string is NULL-terminated.
	dirent.ino = rootNodes[index-1]->inode;
	return &dirent;
*/
}

static fsNode_t *memfs_finddir(fsNode_t *node, char *name)
{
/*
	int i;
	for (i = 0; i < rootNodeCount; i++)
		if (!strcmp(name, rootNodes[i]->name))
			return &rootNodes[i];
	return 0;
*/
}

fsNode_t *memfs_init(uint32 location)
{
/*	log("memfs: Initializing at 0x%x\n", location);
	// Initialise the main and file header pointers and populate the root directory.
	memfsHeader = (memfsHeader_t *)location;
	memfsHeaders = (memfsFileHeader_t *) (location+sizeof(memfsHeader_t));
	DUMPVAR("0x%x", location);
	DUMPVAR("%d", memfsHeader->fileCount);


	int i;
	for (i = 0; i < memfsHeader->fileCount; i++)
	{
		log("memfs: found file.\n");
		// Edit the file's header - currently it holds the file offset
		// relative to the start of the ramdisk. We want it relative to the start
		// of memory.
		memfsHeaders[i].offset += location;
		// Create a new file node.
		strcpy(rootNodes[i].name, memfsHeaders[i].name);
		rootNodes[i].mask = rootNodes[i].uid = rootNodes[i].gid = 0;
		rootNodes[i].length = memfsHeaders[i].length;
		rootNodes[i].inode = i;
		rootNodes[i].flags = FS_FILE;
		rootNodes[i].read = &memfs_read;
		rootNodes[i].write = 0;
		rootNodes[i].readdir = 0;
		rootNodes[i].finddir = 0;
		rootNodes[i].open = 0;
		rootNodes[i].close = 0;
		rootNodes[i].impl = 0;
	}
	*/
}
