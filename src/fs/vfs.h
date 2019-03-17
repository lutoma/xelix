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

// file flags, keep in sync with newlib sys/_default_fcntl.h
#define	O_RDONLY	0
#define	O_WRONLY	1
#define	O_RDWR		2
#define	O_APPEND	0x0008
#define	O_CREAT		0x0200
#define	O_TRUNC		0x0400
#define	O_EXCL		0x0800
#define O_SYNC		0x2000
#define O_NONBLOCK	0x4000

// fcntl operations
#define	F_DUPFD		0	/* Duplicate fildes */
#define	F_GETFD		1	/* Get fildes flags (close on exec) */
#define	F_SETFD		2	/* Set fildes flags (close on exec) */
#define	F_GETFL		3	/* Get file flags */
#define	F_SETFL		4	/* Set file flags */
#define	F_GETOWN 	5	/* Get owner - for ASYNC */
#define	F_SETOWN 	6	/* Set owner - for ASYNC */
#define	F_GETLK  	7	/* Get record-locking information */
#define	F_SETLK  	8	/* Set or Clear a record-lock (Non-Blocking) */
#define	F_SETLKW 	9	/* Set or Clear a record-lock (Blocking) */
#define	F_RGETLK 	10	/* Test a remote lock to see if it is blocked */
#define	F_RSETLK 	11	/* Set or unlock a remote lock */
#define	F_CNVT 		12	/* Convert a fhandle to an open fd */
#define	F_RSETLKW 	13	/* Set or Clear remote record-lock(Blocking) */
#define	F_DUPFD_CLOEXEC	14	/* As F_DUPFD, but set close-on-exec flag */

#define vfs_mode_to_filetype(mode) (mode & 0xf000)

// Can't include <tasks/task.h> as that includes us, so use stub struct def.
struct task;
struct vfs_file;

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
	struct vfs_file* (*open)(char* path, uint32_t flags, void* mount_instance, struct task* task);
	size_t (*read)(struct vfs_file* fp, void* dest, size_t size);
	size_t (*write)(struct vfs_file* fp, void* source, size_t size);
	size_t (*getdents)(struct vfs_file* fp, void* dest, size_t size);
	int (*stat)(struct vfs_file* fp, vfs_stat_t* dest);
	int (*mkdir)(const char* path, uint32_t mode);
	int (*symlink)(const char* path1, const char* path2);
	int (*unlink)(char* name);
	int (*chmod)(const char* path, uint32_t mode);
	int (*chown)(const char* path, uint16_t owner, uint16_t group);
	int (*utimes)(const char* path, struct timeval times[2]);
	int (*link)(const char* path, const char* new_path);
	int (*readlink)(const char* orig_path, char* buf, size_t size);
	int (*rmdir)(char* path);
	int (*ioctl)(const char* path, int request, void* arg);
};

typedef struct vfs_file {
	enum {
		VFS_FILE_TYPE_REG,
		VFS_FILE_TYPE_PIPE,
		VFS_FILE_TYPE_SOCKET,
	} type;

	uint32_t num;
	char path[512];
	char mount_path[512];
	void* mount_instance;
	struct vfs_callbacks callbacks;
	uint32_t flags;
	uint32_t offset;
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

char* vfs_normalize_path(const char* orig_path, char* cwd);
vfs_file_t* vfs_get_from_id(int id, struct task* task);
vfs_file_t* vfs_alloc_fileno(struct task* task);
int vfs_open(const char* orig_path, uint32_t flags, struct task* task);
size_t vfs_read(int fd, void* dest, size_t size, struct task* task);
size_t vfs_write(int fd, void* source, size_t size, struct task* task);
size_t vfs_getdents(int fd, void* dest, size_t size, struct task* task);
int vfs_seek(int fd, size_t offset, int origin, struct task* task);
int vfs_close(int fd, struct task* task);
int vfs_fcntl(int fd, int cmd, int arg3, struct task* task);
int vfs_ioctl(int fd, int request, void* arg, struct task* task);
int vfs_unlink(char* orig_path, struct task* task);
int vfs_chmod(const char* orig_path, uint32_t mode, struct task* task);
int vfs_chown(const char* orig_path, uint16_t uid, uint16_t gid, struct task* task);
int vfs_mkdir(const char* orig_path, uint32_t mode, struct task* task);
int vfs_access(const char* orig_path, uint32_t amode, struct task* task);
int vfs_utimes(const char* orig_path, struct timeval times[2], struct task* task);
int vfs_link(const char* orig_path, const char* orig_new_path, struct task* task);
int vfs_readlink(const char* orig_path, char* buf, size_t size, struct task* task);
int vfs_rmdir(const char* orig_path, struct task* task);
int vfs_mount(char* path, void* instance, char* dev, char* type,
	struct vfs_callbacks* callbacks);

// FIXME Should operate on paths, not open files
int vfs_stat(int fd, vfs_stat_t* dest, struct task* task);

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
