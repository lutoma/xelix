/* Copyright © 2013-2015 Lukas Martini
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
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/dirent.h>
#include <sys/socket.h>
#include <stdio.h>
#include <termios.h>
#include <signal.h>
#include "call.h"

#undef errno
extern int errno;

char *__env[1] = { 0 }; 
//char **environ = __env; 

void _exit(int return_code)
{
	call_exit(return_code);
}

int close(int file)
{
	errno = ENOSYS;
	return -1;
}

int execve(char *name, char **argv, char **env)
{
	fprintf(stderr, "libc: Warning: Call to unimplemented execve()\n");
	errno = ENOSYS;
	call_execve(name, argv, env);
	return -1;
}

int fork()
{
	// Todo: Set proper errno in case something goes wonky
	errno = ENOSYS;
	return call_fork();
}

int fstat(int file, struct stat *st)
{
	errno = ENOSYS;
	return -1;
}

int getpid()
{
	return call_getpid();
}

int isatty(int file)
{
	errno = ENOSYS;
	return 0;
}

int kill(int pid, int sig)
{
	int ret = call_kill(pid, sig);

	switch(ret)
	{
		case -1: errno = ENOSYS;;
		case -2: errno = EINVAL;;
		case -3: errno = EPERM;;
		case -4: errno = ESRCH;;
	}

	return ret;
}

int link(char *old, char *new){
	errno = ENOSYS;
	return -1;
}

int lseek(int file, int ptr, int dir)
{
	int ret = call_seek(file, ptr, dir);

	if(ret == -2)
		errno = ENOENT;
	else if(ret < 1)
		errno = ENOSYS;

	return -1;
}

int open(const char* name, int flags, ...)
{
	// Filter out (obviously) empty paths
	if(name[0] == '\0' || !strcmp(name, " "))
	{
		fprintf(stderr, "libc: Warning: Call to open() with empty path.\n");
		errno = ENOENT;
		return -1;
	}

	int fd = call_open((char*)name);
	if(fd == -1)
		errno = ENOENT;

	return fd;
}

int read(int file, char *buf, int len)
{
	return call_read(file, (void *)buf, (unsigned int)len);
}

int readlink(const char *path, char *buf, size_t bufsize)
{
	errno = ENOSYS;
	return -1;
}

void* sbrk(int incr)
{
	return call_mmap(incr);
}

int stat(const char *name, struct stat *st)
{
	st->st_dev = 1;
	st->st_ino = 1;
	st->st_mode = S_IFREG | S_IRUSR | S_IWUSR | S_IXUSR;
	st->st_nlink = 0;
	st->st_uid = 1;
	st->st_gid = 1;
	st->st_rdev = 0;
	st->st_size = 0x1000;
	st->st_blksize = 512;
	st->st_blocks = 16;
	st->st_atime = 0;
	st->st_mtime = 0;
	st->st_ctime = 0;
	return 0;
}

int symlink(const char *path1, const char *path2)
{
	errno = ENOSYS;
	return -1;
}

clock_t times(struct tms *buf)
{
	errno = ENOSYS;
	return -1;
}

int unlink(char *name)
{
	errno = ENOSYS;
	return -1;
}

int wait(int *status)
{
	errno = ENOSYS;
	return -1;
}

int wait3(int *status)
{
	errno = ENOSYS;
	return -1;
}

int write(int file, char *buf, int len)
{
	return call_write(file, (void *)buf, (unsigned int)len);
}

int closedir(DIR* dd)
{
	errno = ENOSYS;
	return -1;
}

DIR* opendir(const char* path)
{
	errno = ENOSYS;
	return NULL;
}

struct dirent* readdir(DIR* dd)
{
	errno = ENOSYS;
	return NULL;
}

int readdir_r(DIR* dd, struct dirent* de, struct dirent** de2)
{
	errno = ENOSYS;
	return -1;
}

void rewinddir(DIR* dd)
{
	errno = ENOSYS;
	return;
}

void seekdir(DIR* dd, long int sd)
{
	errno = ENOSYS;
	return;
}

long int telldir(DIR* dd)
{
	errno = ENOSYS;
	return -1;
}

speed_t cfgetospeed(const struct termios *termios_p)
{
	errno = ENOSYS;
	return -1;
}

int mkdir(const char *dir_path, mode_t mode) {
	errno = ENOSYS;
	return -1;
}

char* getcwd(char *buf, size_t size)
{
	return call_getcwd(buf, size);
}

char* getwd(char *buf)
{
	return "/";
}

int chdir(const char *path)
{
	return call_chdir(path);
}

int gettimeofday(struct timeval *__p, void *__tz)
{
	errno = ENOSYS;
	return -1;
}

int access(const char *pathname, int mode)
{
	errno = ENOSYS;
	return -1;
}

int tcgetattr(int fd, struct termios *termios_p)
{
	errno = ENOSYS;
	return -1;
}

int tcsetattr(int fd, int optional_actions, const struct termios* termios_p)
{
	errno = ENOSYS;
	return -1;
}

char* ttyname(int desc)
{
	return "/dev/tty0";
}

uid_t getuid(void)
{
	errno = ENOSYS;
	return -1;
}

uid_t geteuid(void)
{
	errno = ENOSYS;
	return -1;
}

uid_t getgid(void)
{
	errno = ENOSYS;
	return -1;
}

uid_t getegid(void)
{
	errno = ENOSYS;
	return -1;
}

int getgroups(int gidsetsize, gid_t grouplist[])
{
	errno = ENOSYS;
	return -1;
}

pid_t getpgrp(void)
{
	// Simply return 1 as per POSIX getpgrp has no return code to indicate an
	// error
	return 1;
}

int sigprocmask(int how, const sigset_t *set, sigset_t *oset)
{
	errno = ENOSYS;
	return -1;
}

int dup(int fildes)
{
	errno = ENOSYS;
	return -1;
}

int dup2(int fildes, int fildes2)
{
	errno = ENOSYS;
	return -1;
}

int pipe(int fildes[2])
{
	errno = ENOSYS;
	return -1;
}

int sigaction(int sig, const struct sigaction* act, struct sigaction* oact)
{
	errno = ENOSYS;
	return -1;
}

int lstat(const char* path, struct stat* buf)
{
	errno = ENOSYS;
	return -1;
}

pid_t tcgetpgrp(int fildes)
{
	errno = ENOSYS;
	return -1;
}

int setpgid(pid_t pid, pid_t pgid)
{
	errno = ENOSYS;
	return -1;
}

int fcntl(int fildes, int cmd, ...)
{
	errno = ENOSYS;
	return -1;
}

int sigsuspend(const sigset_t *sigmask)
{
	errno = ENOSYS;
	return -1;
}

mode_t umask(mode_t cmask)
{
	errno = ENOSYS;
	return -1;
}

pid_t getppid(void)
{
	// Simply return 0 as per POSIX getpgrp has no return code to indicate an
	// error
	return 0;
}

int tcsetpgrp(int fildes, pid_t pgid_id)
{
	errno = ENOSYS;
	return -1;
}

int socket(int domain, int type, int protocol) 
{
	if(domain != AF_INET) {
		errno = EAFNOSUPPORT;
		return -1;
	}

	if(type != SOCK_DGRAM) {
		errno = EPROTOTYPE;
		return -1;
	}

	printf("libc/Xelix: socket() dummy call, returning 0\n");
	return 0;
}

int bind(int socket, const struct sockaddr *address, socklen_t address_len)
{
	// Socket should always be 0 for now as that's all socket() will return
	if(socket != 0) {
		errno = ENOTSOCK;
		return -1;
	}

	printf("libc/Xelix: bind() dummy call, returning 0\n");
	return 0;
}

ssize_t recvfrom(int socket, void *buffer, size_t length, int flags,
	struct sockaddr *address, socklen_t *address_len)
{
	// Socket should always be 0 for now as that's all socket() will return
	if(socket != 0) {
		errno = ENOTSOCK;
		return -1;
	}

	printf("libc/Xelix: recvfrom() dummy call, returning 0\n");
	return 0;
}

ssize_t sendto(int socket, const void *message, size_t length, int flags,
	const struct sockaddr *dest_addr, socklen_t dest_len)
{
	// Socket should always be 0 for now as that's all socket() will return
	if(socket != 0) {
		errno = ENOTSOCK;
		return -1;
	}
	
	printf("libc/Xelix: recvfrom() dummy call, returning %d\n", length);
	return length;
}