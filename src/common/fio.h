#pragma once

/* Copyright © 2010, 2011 Lukas Martini
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

#include <common/generic.h>

#include <filesystems/vfs.h>

typedef struct FILE
{
	const char* path;
	const char* modes;
	fsNode_t* node;
	uint32 position;
} FILE;

FILE *fopen(const char* path, const char* mode);
//FILE *freopen(const char *path, const char *mode, FILE *fp);
int fclose(FILE* fp);
char fgetc(FILE* fp);
size_t fwrite (const void *array, size_t size, size_t count, FILE *stream);
int fputc(int c, FILE *fp);
void rewind(FILE *fp);
long ftell(FILE* fp);
int scandir(const char* dirp, struct dirent*** namelist, int (*filter)(const struct dirent *), int (*compar)(const struct dirent **, const struct dirent **));
