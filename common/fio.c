// File input / output

#include <common/fio.h>

#include <common/log.h>
#include <filesystems/vfs.h>
#include <memory/kmalloc.h>
#include <common/string.h>

// Caution: Some^W most of those functions are untested, beware of the bugs ;)

// Get Node from Path, return NULL if error
fsNode_t* fio_pathToNode(const char* __path)
{
	// Split path and iterate trough the single parts, going from / upwards.
	static char* __pch;
	__pch = strtok(__path, "/");	
	fsNode_t* __node = vfs_rootNode->finddir(vfs_rootNode, __pch);

	while(__pch != NULL && __node != NULL)
	{
		__pch = strtok(NULL, "/");		
		__node = vfs_rootNode->finddir(__node, __pch);
	}
	return __node;
}

FILE* fopen(const char* __path, const char* __modes)
{
	log("fio: Opening file %s, modes '%s'\n", __path, __modes);

	fsNode_t* __node = fio_pathToNode(__path);
	if(__node == NULL)
		return;

	// Tell the driver of the device this file is on we want to open it
	if(__node->open != NULL)
		if(__node->open(__node) == 1)
			return NULL;

	FILE* fp = kmalloc(sizeof(FILE));
	fp->path = __path;
	fp->modes = __modes;
	fp->node = __node;
	fp->position = 0;
	return fp;
}

int fclose(FILE* __fp)
{
	log("fio: Closing file %s\n", __fp->path);
	
	// Tell the driver of the device this file is on we want to close it
	if(__fp->node->close != NULL)
		__fp->node->close(__fp->node);

	kfree(__fp);
}

char fgetc(FILE* fp)
{
	if(fp->node->read == NULL)
		return EOF;
	static char c[1];
	size_t __size = fp->node->read(fp->node, fp->position, 1, c);
	fp->position++;
	return c[0];
}

int fputc(int c, FILE* fp)
{
	if(fp->node->write == NULL)
		return 1;

	static char __c[1];
	return fp->node->write(fp->node, fp->position, 1, __c);
}
/*
size_t fwrite (const void *array, size_t size, size_t count, FILE *fp)
{
	if(fp->node->write == NULL)
		return 1;

	static char __c[1];
	return fp->node->write(fp->node, fp->position, 1, __c);
}
*/
void rewind(FILE* fp)
{
	fp->position = 0;
}

int scandir(const char* dirp, struct dirent*** namelist, int (*filter)(const struct dirent *), int (*compar)(const struct dirent **, const struct dirent **))
{
	return 0;
}
