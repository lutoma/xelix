#include <filesystems/interface.h>

fsNode_t *fsRoot = 0; // The root of the filesystem.

uint32 readFs(fsNode_t *node, uint32 offset, uint32 size, uint8 *buffer)
{
	// Has the node got a read callback?
	if (node->read != 0)
		return node->read(node, offset, size, buffer);
	else
		return 0;
}

uint32 writeFs(fsNode_t *node, uint32 offset, uint32 size, uint8 *buffer)
{
	// Has the node got a write callback?
	if (node->write != 0)
		return node->read(node, offset, size, buffer);
	else
		return 0;
}

void openFs(fsNode_t *node, uint8 read, uint8 write)
{
	// Has the node got a open callback?
	if (node->open != 0)
		return node->open(node);
	else
		return;
}

void closeFs(fsNode_t *node)
{
	// Has the node got a close callback?
	if (node->close != 0)
		return node->close(node);
	else
		return;
}

struct dirent *readdirFs(fsNode_t *node, uint32 index)
{
	// Has the node got a readdir callback and _is_ it actually a directory
	if ((node->flags&0x7) == FS_DIRECTORY && node->readdir != 0 )
		return node->readdir(node, index);
	else
		return 0;
}

fsNode_t *finddirFs(fsNode_t *node, char *name)
{
	// Has the node got a finddir callback and _is_ it actually a directory
	if ((node->flags&0x7) == FS_DIRECTORY && node->finddir != 0 )
		return node->finddir(node, name);
	else
		return 0;
}
