/* Copyright © 2013-2019 Lukas Martini
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

#include <sys/stat.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/dirent.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/errno.h>
#include <sys/xelix.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <signal.h>
#include <string.h>
#include <sgtty.h>
#include <limits.h>
#include <poll.h>
#include <utime.h>

/* Normally errno is defined as a macro that does reentrancy magic. Our
 * syscalls only get called from the reentrant mappings in reent/ and syscall/,
 * so we need to use the system-dependent plain errno.
 */
#undef errno
extern int errno;

#define syscall(call, a1, a2, a3) __syscall(call, (uint32_t)a1, (uint32_t)a2, (uint32_t)a3)

static inline uint32_t __syscall(uint32_t call, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
	register uint32_t _call asm("eax") = call;
	register uint32_t _arg1 asm("ebx") = arg1;
	register uint32_t _arg2 asm("ecx") = arg2;
	register uint32_t _arg3 asm("edx") = arg3;
	register uint32_t result asm("eax");
	register uint32_t sce asm("ebx");

	asm volatile(
		"int $0x80;"

		: "=r" (result), "=r" (sce)
		: "r" (_call), "r" (_arg1), "r" (_arg2), "r" (_arg3)
		: "memory");

	errno = sce;
	return result;
}

void _exit(int return_code) {
	syscall(1, return_code, 0, 0);
}

int _fork() {
	return syscall(22, 0, 0, 0);
}

pid_t _getpid() {
	return (pid_t)_xelix_execdata->pid;
}

pid_t getppid(void) {
	return (pid_t)_xelix_execdata->ppid;
}

int _kill(int pid, int sig) {
	int ret = syscall(18, pid, sig, 0);

	switch(ret)
	{
		case -1: errno = ENOSYS;;
		case -2: errno = EINVAL;;
		case -3: errno = EPERM;;
		case -4: errno = ESRCH;;
	}

	return ret;
}

int _lseek(int file, int ptr, int dir) {
	return syscall(15, file, ptr, dir);
}

int _open(const char* name, int flags, ...) {
	return syscall(13, name, flags, 0);
}

int _close(int file) {
	return syscall(5, file, 0, 0);
}

ssize_t _read(int file, char *buf, int len) {
	return syscall(2, file, buf, len);
}

void* _sbrk(int incr) {
	return (void*)syscall(7, 0, incr, 0);
}

int _wait(int* status) {
	return syscall(29, status, 0, 0);
}

pid_t waitpid(pid_t pid, int* stat_loc, int options) {
	return syscall(29, stat_loc, 0, 0); // FIXME Just the wait syscall
}

int wait3(int* status) {
	return wait(status);
}

int _write(int file, char *buf, int len) {
	return syscall(3, file, buf, len);
}

int chdir(const char *path) {
	int r = syscall(20, path, 0, 0);
	*__errno() = errno;
	return r;
}

int socket(int domain, int type, int protocol) {
	if(domain != AF_INET) {
		errno = EAFNOSUPPORT;
		return -1;
	}

	if(type != SOCK_DGRAM) {
		errno = EPROTOTYPE;
		return -1;
	}

	return syscall(24, domain, type, protocol);
}

int bind(int socket, const struct sockaddr *address, socklen_t address_len) {
	return syscall(25, socket, address, address_len);
}

ssize_t recvfrom(int socket, void *buffer, size_t length, int flags,
	struct sockaddr *address, socklen_t *address_len) {
	return syscall(27, socket, buffer, length);
}

ssize_t recv(int socket, void *buffer, size_t length, int flags) {
	return syscall(27, socket, buffer, length);
}

ssize_t sendto(int socket, const void *message, size_t length, int flags,
	const struct sockaddr *dest_addr, socklen_t dest_len) {
	return syscall(26, socket, message, length);
}

ssize_t send(int socket, const void *buffer, size_t length, int flags) {
	return syscall(26, socket, buffer, length);
}

int _execve(char *name, char **argv, char **env) {
	return syscall(32, name, argv, env);
}

// Gets called by the newlib readdir handler, see libc/posix/readdir.c
int getdents(unsigned int fd, struct dirent** dirp, unsigned int count) {
	return syscall(16, fd, dirp, count);
}

int gethostname(char *name, size_t namelen) {
	strncpy(name, "localhost", namelen);
	FILE* fp = fopen("/etc/hostname", "r");
	if(fp) {
		char hostname[300];
		if(fscanf(fp, "%s\n", &hostname) == 1) {
			strncpy(name, hostname, namelen);
		}
		fclose(fp);
	}

	return 0;
}

int uname(struct utsname* name) {
	strcpy(name->sysname, "Xelix");
	gethostname(name->nodename, 300);
	strcpy(name->release, "alpha");
	strcpy(name->version, "0.0.1");
	strcpy(name->machine, "i786");
	return 0;
}

int _fstat(int file, struct stat* st) {
	return syscall(14, file, st, 0);
}

int _stat(const char* name, struct stat *st) {
	int fp = open(name, O_RDONLY);
	if(fp == -1) {
		return -1;
	}

	int r = _fstat(fp, st);
	int stat_errno = errno;
	_close(fp);
	errno = stat_errno;
	return r;
}

int lstat(const char* name, struct stat *st) {
	return stat(name, st);
}

int mkdir(const char *dir_path, mode_t mode) {
	return syscall(6, dir_path, mode, 0);
}

int symlink(const char *path1, const char *path2) {
	return syscall(8, path1, path2, 0);
}

int _unlink(char *name) {
	return syscall(10, name, 0, 0);
}

int chmod(const char *path, mode_t mode) {
	return syscall(11, path, mode, 0);
}

int chown(const char *path, uid_t owner, gid_t group) {
	return syscall(17, path, owner, group);
}

int access(const char *pathname, int mode) {
	return syscall(4, pathname, mode, 0);
}

int _gettimeofday(struct timeval* p, void* tz) {
	return syscall(19, p, tz, 0);
}

int utimes(const char *path, const struct timeval times[2]) {
	return syscall(21, path, times, 0);
}

int utime(const char *path, const struct utimbuf *times) {
	struct timeval* times_arr = NULL;
	if(times) {
		times_arr = (struct timeval*)calloc(2, sizeof(struct timeval));
		if(!times_arr) {
			return -1;
		}

		times_arr[0].tv_sec = times->actime;
		times_arr[1].tv_sec = times->modtime;
	}

	int r = utimes(path, times_arr);
	int utimes_errno = errno;
	if(times_arr) {
		free(times_arr);
	}
	errno = utimes_errno;
	return r;
}

int rmdir(const char *path) {
	return syscall(23, path, 0, 0);
}

int _link(char *old, char *new){
	return syscall(12, old, new, 0);
}

int readlink(const char *path, char *buf, size_t bufsize) {
	return syscall(31, path, buf, bufsize);
}
