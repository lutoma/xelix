#pragma once

/* Copyright © 2010-2018 Lukas Martini
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

#include <lib/generic.h>

#define VFS_SEEK_SET 0
#define VFS_SEEK_CUR 1
#define VFS_SEEK_END 2

// File Types
#define FT_IFSOCK	0xC000
#define FT_IFLNK	0xA000
#define FT_IFREG	0x8000
#define FT_IFBLK	0x6000
#define FT_IFDIR	0x4000
#define FT_IFCHR	0x2000
#define FT_IFIFO	0x1000

// Permissions
#define S_IRUSR		0x0100
#define S_IWUSR		0x0080
#define S_IXUSR		0x0040
#define S_IRGRP		0x0020
#define S_IWGRP		0x0010
#define S_IXGRP		0x0008
#define S_IROTH		0x0004
#define S_IWOTH		0x0002
#define S_IXOTH		0x0001

#define vfs_mode_to_filetype(mode) (mode & 0xf000)

typedef struct {
   uint32_t num;
   char path[512];
   char mount_path[512];
   void* mount_instance;
   uint32_t offset;
   uint32_t mountpoint;
   uint32_t inode;
} vfs_file_t;


/* This is currently directly used for ext2 reads. Format should not be changed
 * unless it is removed from there.
 */
typedef struct {
	uint32_t inode;
	uint16_t record_len;
	uint8_t name_len;
	uint8_t type;
	char name[];
} __attribute__((packed)) vfs_dirent_t;

typedef struct {
	uint32_t st_dev;
	uint32_t st_ino;
	uint32_t st_mode;
	uint32_t st_nlink;
	uint16_t st_uid;
	uint16_t st_gid;
	uint32_t st_rdev;
	uint32_t st_size;
	uint32_t st_atime;
	uint32_t st_mtime;
	uint32_t st_ctime;
} __attribute__((packed)) vfs_stat_t;


typedef uint32_t (*vfs_open_callback_t)(char* path, void* mount_instance);
typedef int (*vfs_stat_callback_t)(vfs_file_t* fp, vfs_stat_t* dest);
typedef size_t (*vfs_read_callback_t)(vfs_file_t* fp, void* dest, size_t size);
typedef size_t (*vfs_getdents_callback_t)(vfs_file_t* fp, void* dest, size_t size);


// Used to always store the last read/write attempt (used for kernel panic debugging)
char vfs_last_read_attempt[512];

char* vfs_filetype_to_verbose(int filetype);
char* vfs_get_verbose_permissions(uint32_t mode);
vfs_file_t* vfs_get_from_id(uint32_t id);
vfs_file_t* vfs_open(char* path);
int vfs_stat(vfs_file_t* fp, vfs_stat_t* dest);
size_t vfs_read(void* dest, size_t size, vfs_file_t* fp);
size_t vfs_getdents(vfs_file_t* dir, void* dest, size_t size);
void vfs_seek(vfs_file_t* fp, size_t offset, int origin);

int vfs_mount(char* path, void* instance, char* dev, char* type,
	vfs_open_callback_t open_callback, vfs_stat_callback_t stat_callback,
	vfs_read_callback_t read_callback, vfs_getdents_callback_t getdents_callback);

void vfs_init();
