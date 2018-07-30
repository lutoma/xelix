/* Copyright © 2013-2018 Lukas Martini
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
#include <sys/time.h>
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

#define syscall(call, a1, a2, a3) __syscall(call, (uint32_t)a1, (uint32_t)a2, (uint32_t)a3)

static inline uint32_t __syscall(uint32_t call, uint32_t arg1, uint32_t arg2, uint32_t arg3) {
	register uint32_t _call asm("eax") = call;
	register uint32_t _arg1 asm("ebx") = arg1;
	register uint32_t _arg2 asm("ecx") = arg2;
	register uint32_t _arg3 asm("edx") = arg3;
	register uint32_t result asm("eax");

	asm volatile("int $0x80;" : "=r" (result) : "r" (_call), "r" (_arg1), "r" (_arg2), "r" (_arg3));
	return result;
}

int _xelix_getexecdata() {
	_xelix_execdata = (struct xelix_execdata*)malloc(0x400);
	syscall(19, _xelix_execdata, 0x400, 0);
	return 0;
}

void _exit(int return_code) {
	syscall(1, return_code, 0, 0);
}

int fork() {
	// Todo: Set proper errno in case something goes wonky
	errno = ENOSYS;
	return syscall(22, 0, 0, 0);
}

pid_t getpid() {
	return (pid_t)_xelix_execdata->pid;
}

pid_t getppid(void) {
	return (pid_t)_xelix_execdata->ppid;
}

int kill(int pid, int sig) {
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

int lseek(int file, int ptr, int dir) {
	int ret = syscall(14, file, ptr, dir);

	if(ret == -2)
		errno = ENOENT;
	else if(ret < 1)
		errno = ENOSYS;

	return -1;
}

int open(const char* name, int flags, ...) {
	// Filter out (obviously) empty paths
	if(name[0] == '\0' || !strcmp(name, " "))
	{
		fprintf(stderr, "libc: Warning: Call to open() with empty path.\n");
		errno = ENOENT;
		return -1;
	}

	if(flags & O_WRONLY) {
		errno = ENOSYS;
		return -1;
	}

	int fd = syscall(13, name, 0, 0);
	if(fd == -1)
		errno = ENOENT;

	return fd;
}

ssize_t read(int file, char *buf, int len) {
	int read = syscall(2, file, buf, len);
	/*if (!read) {
		errno = EINTR;
		return -1;
	}*/

	return read;
}

void* sbrk(int incr) {
	return (void*)syscall(7, 0, incr, 0);
}

int wait(int* status) {
	return syscall(29, status, 0, 0);
}

pid_t waitpid(pid_t pid, int* stat_loc, int options) {
	return syscall(29, stat_loc, 0, 0); // FIXME Just the wait syscall
}

int write(int file, char *buf, int len) {
	return syscall(3, file, buf, len);
}

int chdir(const char *path) {
	return syscall(20, path, 0, 0);
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

pid_t execnew(const char* path, char* __argv[], char* __env[]) {
	return syscall(28, (uint32_t)path, (uint32_t)__argv, (uint32_t)__env);
}

int _execve(char *name, char **argv, char **env) {
	errno = ENOSYS;
	return syscall(23, name, argv, env);
}

// Gets called by the newlib readdir handler, see libc/posix/readdir.c
int getdents(unsigned int fd, struct dirent** dirp, unsigned int count) {
	return syscall(16, fd, dirp, count);
}

int uname(struct utsname* name) {
	strcpy(name->sysname, "Xelix");
	strcpy(name->nodename, "default");
	strcpy(name->release, "alpha");
	strcpy(name->version, "0.0.1");
	strcpy(name->machine, "i686");
	return 0;
}

int fstat(int file, struct stat* st) {
	int r = syscall(14, file, st, 0);
	st->st_mode = S_IFDIR;
	return r;
}

int stat(const char* name, struct stat *st) {
	int fp = open(name, 0);
	if(!fp) {
		return -1;
	}

	int r = fstat(fp, st);
	int stat_errno = errno;

	//close(fp);
	errno = stat_errno;
	return r;
}

int lstat(const char* name, struct stat *st) {
	return stat(name, st);
}

int gettimeofday(struct timeval* p, void* tz) {
	return syscall(17, p, tz, 0);
}
