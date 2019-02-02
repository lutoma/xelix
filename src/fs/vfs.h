#pragma once

/* Copyright © 2010-2019 Lukas Martini
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

#include <string.h>
#include <print.h>
#include <time.h>

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

// vfs_open flags, Keep in sync with newlib sys/_default_fcntl.h
#define	O_RDONLY	0
#define	O_WRONLY	1
#define	O_RDWR		2
#define	O_APPEND	0x0008
#define	O_CREAT		0x0200
#define	O_TRUNC		0x0400
#define	O_EXCL		0x0800
#define O_SYNC		0x2000

#define vfs_mode_to_filetype(mode) (mode & 0xf000)

// Can't include <tasks/task.h> as that includes us, so use stub struct def.
struct task;

typedef struct {
   uint32_t num;
   char path[512];
   char mount_path[512];
   void* mount_instance;
   uint32_t flags;
   uint32_t offset;
   uint32_t mountpoint;
   uint32_t inode;
   struct task* task;
} vfs_file_t;

// Keep in sync with newlib
typedef struct {
	uint32_t d_ino;
	uint32_t d_off;
	uint16_t d_reclen;
    uint8_t d_type;
	char d_name[] __attribute__ ((nonstring));
} vfs_dirent_t;

// Keep in sync with newlib
typedef struct {
	uint16_t st_dev;
	uint16_t st_ino;
	uint32_t st_mode;
	uint16_t st_nlink;
	uint16_t st_uid;
	uint16_t st_gid;
	uint16_t st_rdev;
	uint32_t st_size;
	uint64_t st_atime;
	long st_spare1;
	uint64_t st_mtime;
	long st_spare2;
	uint64_t st_ctime;
	long st_spare3;
	uint32_t st_blksize;
	uint32_t st_blocks;
	long st_spare4[2];
} __attribute__((packed)) vfs_stat_t;

struct vfs_callbacks {
	uint32_t (*open)(char* path, uint32_t flags, void* mount_instance);
	size_t (*read)(vfs_file_t* fp, void* dest, size_t size);
	size_t (*write)(vfs_file_t* fp, void* source, size_t size);
	size_t (*getdents)(vfs_file_t* fp, void* dest, size_t size);
	int (*stat)(vfs_file_t* fp, vfs_stat_t* dest);
	int (*mkdir)(const char* path, uint32_t mode);
	int (*symlink)(const char* path1, const char* path2);
	int (*unlink)(char* name);
	int (*chmod)(const char* path, uint32_t mode);
	int (*chown)(const char* path, uint16_t owner, uint16_t group);
	int (*utimes)(const char* path, struct timeval times[2]);
	int (*rmdir)(const char* path);
};

// Used to always store the last read/write attempt (used for kernel panic debugging)
char vfs_last_read_attempt[512];

char* vfs_normalize_path(const char* orig_path, char* cwd);
vfs_file_t* vfs_get_from_id(int id, struct task* task);
vfs_file_t* vfs_open(const char* path, uint32_t flags, struct task* task);
int vfs_stat(vfs_file_t* fp, vfs_stat_t* dest);
size_t vfs_read(void* dest, size_t size, vfs_file_t* fp);
size_t vfs_write(void* source, size_t size, vfs_file_t* fp);
size_t vfs_getdents(vfs_file_t* dir, void* dest, size_t size);
void vfs_seek(vfs_file_t* fp, size_t offset, int origin);
int vfs_close(vfs_file_t* fp);
int vfs_unlink(char *name, struct task* task);
int vfs_chmod(const char* path, uint32_t mode, struct task* task);
int vfs_mkdir(const char* orig_path, uint32_t mode, struct task* task);
int vfs_access(const char *path, uint32_t amode, struct task* task);
int vfs_utimes(const char* orig_path, struct timeval times[2], struct task* task);
int vfs_rmdir(const char* orig_path, struct task* task);

int vfs_mount(char* path, void* instance, char* dev, char* type,
	struct vfs_callbacks* callbacks);

void vfs_init();


static inline char* vfs_filetype_to_verbose(int filetype) {
	switch(filetype) {
		case FT_IFSOCK: return "FT_IFSOCK";
		case FT_IFLNK: return "FT_IFLNK";
		case FT_IFREG: return "FT_IFREG";
		case FT_IFBLK: return "FT_IFBLK";
		case FT_IFDIR: return "FT_IFDIR";
		case FT_IFCHR: return "FT_IFCHR";
		case FT_IFIFO: return "FT_IFIFO";
		default: return NULL;
	}
}

static inline char* vfs_get_verbose_permissions(uint32_t mode) {
	char* permstring = "         ";
	permstring[0] = (mode & S_IRUSR) ? 'r' : '-';
	permstring[1] = (mode & S_IWUSR) ? 'w' : '-';
	permstring[2] = (mode & S_IXUSR) ? 'x' : '-';
	permstring[3] = (mode & S_IRGRP) ? 'r' : '-';
	permstring[4] = (mode & S_IWGRP) ? 'w' : '-';
	permstring[5] = (mode & S_IXGRP) ? 'x' : '-';
	permstring[6] = (mode & S_IROTH) ? 'r' : '-';
	permstring[7] = (mode & S_IWOTH) ? 'w' : '-';
	permstring[8] = (mode & S_IXOTH) ? 'x' : '-';
	permstring[9] = 0;
	return permstring;
}

static inline char* vfs_flags_verbose(uint32_t flags) {
	char* mode = "O_RDONLY";

	if(flags & O_WRONLY) {
		mode = "O_WRONLY";
	} else if(flags & O_RDWR) {
		mode ="O_RDWR";
	}

	return mode;
}

static inline char* vfs_basename(char* path) {
	char* bname = path + strlen(path);
	while(*(bname - 1) != '/' && bname >= path) { bname--; }
	return bname;
}
