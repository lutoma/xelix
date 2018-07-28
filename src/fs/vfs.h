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

#include <lib/generic.h>

#define VFS_SEEK_SET 0
#define VFS_SEEK_CUR 1
#define VFS_SEEK_END 2

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

typedef uint32_t (*vfs_open_callback_t)(char* path, void* mount_instance);
typedef size_t (*vfs_read_callback_t)(vfs_file_t* fp, void* dest, size_t size);
typedef size_t (*vfs_getdents_callback_t)(vfs_file_t* fp, void* dest, size_t size);


// Used to always store the last read/write attempt (used for kernel panic debugging)
char vfs_last_read_attempt[512];

vfs_file_t* vfs_get_from_id(uint32_t id);
vfs_file_t* vfs_open(char* path);
size_t vfs_read(void* dest, size_t size, vfs_file_t* fp);
size_t vfs_getdents(vfs_file_t* dir, void* dest, size_t size);
void vfs_seek(vfs_file_t* fp, size_t offset, int origin);

int vfs_mount(char* path, void* instance, char* dev, char* type,
	vfs_open_callback_t open_callback, vfs_read_callback_t read_callback,
	vfs_getdents_callback_t getdents_callback);
