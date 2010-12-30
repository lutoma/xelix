// File input / output

#include <common/fio.h>

#include <common/log.h>
#include <filesystems/vfs.h>
#include <memory/kmalloc.h>
#include <common/string.h>

// Caution: Some^W most of those functions are untested, beware of the bugs ;)

// Get Node from Path, return NULL if error
fsNode_t* fio_pathToNode(const char* path)
{
	// Split path and iterate trough the single parts, going from / upwards.
	static char* pch;
	pch = strtok(path, "/");	
	fsNode_t* node = vfs_rootNode->finddir(vfs_rootNode, pch);

	while(pch != NULL && node != NULL)
	{
		pch = strtok(NULL, "/");		
		node = vfs_rootNode->finddir(node, pch);
	}
	return node;
}

FILE* fopen(const char* path, const char* modes)
{
	log("fio: Opening file %s, modes '%s'\n", path, modes);

	fsNode_t* node = fio_pathToNode(path);
	if(node == NULL)
		return;

	// Tell the driver of the device this file is on we want to open it
	if(node->open != NULL)
		if(node->open(node) == 1)
			return NULL;

	FILE* fp = kmalloc(sizeof(FILE));
	fp->path = path;
	fp->modes = modes;
	fp->node = node;
	fp->position = 0;
	return fp;
}

int fclose(FILE* fp)
{
	log("fio: Closing file %s\n", fp->path);
	
	// Tell the driver of the device this file is on we want to close it
	if(fp->node->close != NULL)
		fp->node->close(fp->node);

	kfree(fp);
}

char fgetc(FILE* fp)
{
	if(fp->node->read == NULL)
		return EOF;
	static char c[1];
	size_t size = fp->node->read(fp->node, fp->position, 1, c);
	fp->position++;
	return c[0];
}

int fputc(int c, FILE* fp)
{
	if(fp->node->write == NULL)
		return 1;

	static char s[1];
	return fp->node->write(fp->node, fp->position, 1, s);
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

long ftell(FILE* fp)
{
	return fp->position;
}

int scandir(const char* dirp, struct dirent*** namelist, int (*filter)(const struct dirent *), int (*compar)(const struct dirent **, const struct dirent **))
{
	return 0;
}
