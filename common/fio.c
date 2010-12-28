// File input / output

#include <common/fio.h>

#include <common/log.h>
#include <filesystems/vfs.h>
#include <memory/kmalloc.h>
#include <common/string.h>

FILE* fopen(const char* __path, const char* __modes)
{
	log("fio: Opening file %s, modes '%s'\n", __path, __modes);

	// Split path and iterate trough the single parts, going from / upwards.
	char* __pch;
	__pch = strtok(__path,"/");	
	fsNode_t* __node = vfs_finddirNode(rootNode, __pch);

	while(__pch != NULL)
	{
		__pch = strtok(NULL, "/");		
		__node = vfs_finddirNode(__node, __pch);

		if(__node == NULL)
			return NULL;
	}

	// Tell the driver of the device this file is on we want to open it
	if(__node->open != NULL)
		__node->open(__node);

	FILE* fp = kmalloc(sizeof(FILE));
	fp->path = __path;
	fp->modes = __modes;
	fp->node = __node;
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

/*
int fgetc(FILE* fp);
size_t fwrite (const void *array, size_t size, size_t count, FILE *stream);
int fputc(int c, FILE *fp);
*/
