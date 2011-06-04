/* fio.c: File input / output abstraction to the virtual filesystem
 * Copyright © 2010 Lukas Martini
 *
 * This file is part of Xelix.
 *
 * Xelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "fio.h"

#include "log.h"
#include <fs/vfs.h>
#include <memory/kmalloc.h>
#include "string.h"

// Caution: Some^W most of those functions are untested, beware of the bugs ;)

// Get Node from Path, return NULL if error
fsNode_t* fio_pathToNode(const char* origPath)
{
	// Split path and iterate trough the single parts, going from / upwards.
	static char* pch;
	char* sp;

	/* We want a throw-away version of path as strtok is kind of
	 * destructive and increases the pointer
	 */
	char* path = (char*)kmalloc((strlen(origPath) + 1) * sizeof(char));
	memcpy(path, (char*)origPath, sizeof(path) +1);
	
	pch = strtok_r(path, "/", &sp);	
	fsNode_t* node = vfs_rootNode->findDir(vfs_rootNode, pch);

	while(pch != NULL && node != NULL)
	{
		pch = strtok_r(NULL, "/", &sp);
		node = node->findDir(node, pch);
	}
	
	kfree(path);
	return node;
}

FILE* fopen(const char* path, const char* modes)
{
	log("fio: Opening file %s, modes '%s'\n", path, modes);
	
	fsNode_t* node = fio_pathToNode(path);
	if(node == NULL)
		return NULL;

	// Tell the driver of the device this file is on we want to open it
	if(node->open != NULL)
		if(node->open(node) == 1)
			return NULL;

	FILE* fp = (FILE*)kmalloc(sizeof(FILE));
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
	return 0;
}

char fgetc(FILE* fp)
{
	if(fp->node->read == NULL)
		return EOF;
	static uint8_t* c;
	fp->node->read(fp->node, fp->position, 1, c);
	fp->position++;
	return c[0];
}

int fputc(int c, FILE* fp)
{
	if(fp->node->write == NULL)
		return 1;

	static uint8_t* s;
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
