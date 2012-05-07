#pragma once

/* Copyright © 2012 Lukas Martini
 *
 * This file is part of Xlibc.
 *
 * Xlibc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Xlibc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Xlibc. If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <stdint.h>

#define S_IFMT 0
#define S_IFBLK 1
#define S_IFCHR 2
#define S_IFIFO 3
#define S_IFREG 4
#define S_IFDIR 5
#define S_IFLNK 6
#define S_IFSOCK 7

struct stat {
	uint32_t st_dev;
	unsigned long st_ino;
	uint16_t st_mode;
	uint16_t st_nlink;
	uint16_t st_uid;
	uint16_t st_gid;
	uint32_t st_rdev;
	unsigned long st_size;
	unsigned long st_blksize;
	unsigned long st_blocks;
	time_t st_atime;
	unsigned long st_atime_nsec;
	time_t st_mtime;
	unsigned long st_mtime_nsec;
	time_t st_ctime;
	unsigned long st_ctime_nsec;
	unsigned long __unused4;
	unsigned long __unused5;
};

struct stat64 {
	uint64_t st_dev;
	unsigned char __pad0[4];
	unsigned long __st_ino;
	uint32_t st_mode;
	uint32_t st_nlink;
	unsigned long st_uid;
	unsigned long st_gid;
	uint64_t st_rdev;
	unsigned char __pad3[4];
	__extension__ long long	st_size __attribute__((__packed__));
	unsigned long st_blksize;
	uint64_t st_blocks;
	time_t st_atime;
	unsigned long st_atime_nsec;
	time_t st_mtime;
	unsigned long st_mtime_nsec;
	time_t st_ctime;
	unsigned long st_ctime_nsec;
	__extension__ unsigned long long	st_ino __attribute__((__packed__));
};