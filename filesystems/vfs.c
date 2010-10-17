// This file provices abstraction from the filesystem drivers for normal use.
#include <filesystems/vfs.h>

#include <common/log.h>
#include <memory/kmalloc.h>

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
    if ( (node->flags&0x7) == FS_DIRECTORY &&
         node->readdir != 0 )
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


// Initialize the filesystem abstraction system
void fs_init()
{

	log("Initialized filesystem abstraction\n");
}
