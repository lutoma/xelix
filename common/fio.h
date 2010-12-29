#pragma once

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
