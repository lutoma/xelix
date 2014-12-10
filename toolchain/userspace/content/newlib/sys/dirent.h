/* Copyright © 2013 Lukas Martini
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
 * along with Xelix. If not, see <http://www.gnu.org/licenses/>.
 */

#include <limits.h>

#ifndef PATH_MAX
#define PATH_MAX	4096
#endif


#ifndef _SYS_DIRENT_H_
#define _SYS_DIRENT_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef struct DIR {
	int num;
	char* path;
	int offset;
} DIR;

typedef struct dirent {
	ino_t d_ino;
	char d_name[PATH_MAX];
} DIRENT;

int closedir(DIR*);
DIR* opendir(const char*);
struct dirent* readdir(DIR *);
int readdir_r(DIR*, struct dirent*, struct dirent**);
void rewinddir(DIR*);
void seekdir(DIR*, long int);
long int telldir(DIR*);

#ifdef __cplusplus
}
#endif
#endif /*_SYS_DIRENT_H_*/