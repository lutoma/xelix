// This file provices abstraction from the filesystem drivers for normal use.
#include <filesystems/vfs.h>

#include <common/log.h>
#include <memory/kmalloc.h>

fsNode_t *devNode; // We also add a directory node for /dev, so we can mount devfs later on.

uint32 vfs_readNode(fsNode_t *node, uint32 offset, uint32 size, uint8 *buffer)
{
  // Has the node got a read callback?
  if (node->read != 0)
    return node->read(node, offset, size, buffer);
  else
    return 0;
}

void vfs_openNode(fsNode_t *node, uint8 read, uint8 write)
{
    // Has the node got an open callback?
    if (node->open != 0)
        return node->open(node);
}

void vfs_closeNode(fsNode_t *node)
{
    // Has the node got a close callback?
    if (node->close != 0)
        return node->close(node);
}

struct dirent *vfs_readdirNode(fsNode_t *node, uint32 index)
{
	// Is the node a directory, and does it have a callback?
	if ( (node->flags&0x7) == FS_DIRECTORY && node->readdir != 0 )
		return node->readdir(node, index);
	else
		return 0;
}

fsNode_t *vfs_finddirNode(fsNode_t *node, char *name)
{
    // Is the node a directory, and does it have a callback?
    if ( (node->flags&0x7) == FS_DIRECTORY &&
         node->finddir != 0 )
        return node->finddir(node, name);
    else
        return 0;
}

struct dirent dirent;
// Read directory [aka get content]
static struct dirent *dummyReadDir(fsNode_t *node, uint32 index)
{
	if (index >= rootNodeCount)
		return 0;
				
	strcpy(dirent.name, rootNodes[index].name);
	dirent.name[strlen(rootNodes[index].name)] = 0; // Make sure the string is NULL-terminated.
	dirent.ino = rootNodes[index].inode;
	return &dirent;
}

static fsNode_t *dummyFindDir(fsNode_t *node, char *name)
{
	int i;
	for (i = 0; i < rootNodeCount; i++)
		if (!strcmp(name, rootNodes[i].name))
			return &rootNodes[i];
	return 0;
}

// Initialize the filesystem abstraction system
void vfs_init()
{
	// Initialise the root directory.
	rootNode = (fsNode_t*)kmalloc(sizeof(fsNode_t));
	strcpy(rootNode->name, "root");
	rootNode->mask = rootNode->uid = rootNode->gid = rootNode->inode = rootNode->length = 0;
	rootNode->flags = FS_DIRECTORY;
	rootNode->read = 0;
	rootNode->write = 0;
	rootNode->open = 0;
	rootNode->close = 0;
	rootNode->readdir = &dummyReadDir;
	rootNode->finddir = &dummyFindDir;
	rootNode->ptr = 0;
	rootNode->impl = 0;
	
	#define POSIX_DIRNUM 2
	// Create the needed directories as specified by POSIX.
	rootNodes = (fsNode_t*)kmalloc(sizeof(fsNode_t) * POSIX_DIRNUM); //Allocate space for files
	char* posixDirs[POSIX_DIRNUM] = {"tmp", "dev"};
	rootNodeCount = POSIX_DIRNUM;
	
	int i;
	for (i = 0; i < POSIX_DIRNUM; i++)
	{
		// Create a new node.
		strcpy(rootNodes[i].name, posixDirs[i]);
		rootNodes[i].mask = 0;
		rootNodes[i].uid = 0;
		rootNodes[i].gid = 0;
		rootNodes[i].length = 0;
		rootNodes[i].inode = i;
		rootNodes[i].flags = FS_DIRECTORY;
		rootNodes[i].read = 0;
		rootNodes[i].write = 0;
		rootNodes[i].readdir = 0;
		rootNodes[i].finddir = 0;
		rootNodes[i].open = 0;
		rootNodes[i].close = 0;
		rootNodes[i].impl = 0;
	}

	log("vfs: Initialized\n");
}

