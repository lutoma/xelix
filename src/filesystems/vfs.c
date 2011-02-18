// This file provices abstraction from the filesystem drivers for normal use.
#include <filesystems/vfs.h>

#include <common/log.h>
#include <memory/kmalloc.h>
#include <common/string.h>
#include <common/fio.h>
/*
 * // Is the node a directory, and does it have a callback?
 * if ( (node->flags&0x7) == FS_DIRECTORY && node->readdir != 0 )
*/
struct dirent dirent;
// Read directory [aka get content]
static struct dirent *dummyReadDir(fsNode_t* node, uint32 index)
{
	if(node != vfs_rootNode || index > 1) // Note: this is only hardcoded sh*t and should be replaced ASAP!
		return 0;

	strcpy(dirent.name, node->name);
	dirent.name[strlen(node->name)] = 0; // Make sure the string is NULL-terminated.
	dirent.ino = node->inode;
	return &dirent;
}

static fsNode_t *dummyFindDir(fsNode_t *node, char *name)
{
	return 0;
}

fsNode_t* vfs_createNode(char name[128], uint32 mask, uint32 uid, uint32 gid, uint32 flags, uint32 inode, uint32 length, uint32 impl, read_type_t read, write_type_t write, open_type_t open, close_type_t close, readdir_type_t readdir, finddir_type_t finddir, fsNode_t *ptr, fsNode_t *parent)
{
	fsNode_t* node = kmalloc(sizeof(fsNode_t));
	strcpy(node->name, name);
	
	if(parent == NULL)
			parent = node; // This node is it's own parent

	log("vfs: Creating new node %s (flags 0x%x, parent %s).\n", name, flags, parent->name);
	
	node->mask = mask;
	node->uid = uid;
	node->gid = gid;
	node->flags = flags;
	node->inode = inode;
	node->length = length;
	node->impl = impl;
	node->read = read;
	node->write = write;
	node->open = open;
	node->close = close;
	node->readdir = readdir;
	node->finddir = finddir;
	node->ptr = ptr;
	node->parent = parent;

	return node;
}

// Initialize the filesystem abstraction system
void vfs_init(char** modules)
{
	// Load the initrd
	fsNode_t *bla = memfs_init(modules[0]);
}

