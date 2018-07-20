/* Copyright © 2013-2016 Lukas Martini
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
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <termios.h>
#include <signal.h>
#include <string.h>
#include <sgtty.h>

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


void _exit(int return_code)
{
	syscall(1, return_code, 0, 0);
}

int close(int file)
{
	errno = ENOSYS;
	return -1;
}

int execve(char *name, char **argv, char **env)
{
	errno = ENOSYS;
	return syscall(23, name, argv, env);
}

int fork()
{
	// Todo: Set proper errno in case something goes wonky
	errno = ENOSYS;
	return syscall(22, 0, 0, 0);
}

int fstat(int file, struct stat *st)
{
	errno = ENOSYS;
	return -1;
}

int getpid()
{
	return syscall(4, 0, 0, 0);
}

int isatty(int file)
{
	errno = ENOSYS;
	return 0;
}

int kill(int pid, int sig)
{
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

int link(char *old, char *new){
	errno = ENOSYS;
	return -1;
}

int lseek(int file, int ptr, int dir)
{
	int ret = syscall(14, file, ptr, dir);

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

	int fd = syscall(13, name, 0, 0);
	if(fd == -1)
		errno = ENOENT;

	return fd;
}

ssize_t read(int file, char *buf, int len)
{
	return syscall(2, file, buf, len);
}

int readlink(const char *path, char *buf, size_t bufsize)
{
	errno = ENOSYS;
	return -1;
}

void* sbrk(int incr)
{
	return (void*)syscall(7, 0, incr, 0);
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

int wait(int* status)
{
	return syscall(29, status, 0, 0);
}

int wait3(int* status)
{
	errno = ENOSYS;
	return -1;
}

pid_t waitpid(pid_t pid, int* stat_loc, int options) {
	return syscall(29, stat_loc, 0, 0); // FIXME Just the wait syscall
}

int write(int file, char *buf, int len)
{
	return syscall(3, file, buf, len);
}

ssize_t pwrite(int fildes, const void *buf, size_t nbyte, off_t offset) {
	errno = ENOSYS;
	return -1;
}

DIR* opendir(const char* path)
{
	int r = syscall(16, path, 0, 0);
	if(!r) {
		errno = ENOENT;
		return NULL;
	}

	DIR* dir = (DIR*)malloc(sizeof(DIR));
	dir->num = r;
	dir->path = strndup(path, PATH_MAX);
	dir->offset = 0;
	return dir;

}

int closedir(DIR* dir)
{
	free(dir->path);
	free(dir);
	return 0;
}

struct dirent* readdir(DIR* dir)
{
	char* r = (char*)syscall(17, dir->num, dir->offset++, 0);
	if(!r) {
		return NULL;
	}

	struct dirent* ent = malloc(sizeof(struct dirent));
	strncpy(ent->d_name, r, PATH_MAX);
	ent->d_ino = 0; // FIXME
	return ent;
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
	return (char*)syscall(21, buf, size, 0);
}

char* getwd(char *buf)
{
	return getcwd(buf, PATH_MAX);
}

int chdir(const char *path)
{
	return syscall(20, path, 0, 0);
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

int setgroups(size_t size, const gid_t *list) {
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

	return syscall(24, domain, type, protocol);
}

int bind(int socket, const struct sockaddr *address, socklen_t address_len)
{
	return syscall(25, socket, address, address_len);
}

ssize_t recvfrom(int socket, void *buffer, size_t length, int flags,
	struct sockaddr *address, socklen_t *address_len)
{
	return syscall(27, socket, buffer, length);
}

ssize_t recv(int socket, void *buffer, size_t length, int flags) {
	return syscall(27, socket, buffer, length);
}

ssize_t sendto(int socket, const void *message, size_t length, int flags,
	const struct sockaddr *dest_addr, socklen_t dest_len)
{
	return syscall(26, socket, message, length);
}

ssize_t send(int socket, const void *buffer, size_t length, int flags) {
	return syscall(26, socket, buffer, length);
}

pid_t execnew(const char* path, char* __argv[], char* __env[]) {
	return syscall(28, (uint32_t)path, (uint32_t)__argv, (uint32_t)__env);
}

int gtty (int __fd, struct sgttyb *__params) {
	errno = ENOSYS;
	return 0;
}

/* Set the terminal parameters associated with FD to *PARAMS.  */
int stty (int __fd, __const struct sgttyb *__params) {
	errno = ENOSYS;
	return 0;
}

int chroot(const char *path) {
	errno = ENOSYS;
	return -1;
}

int setgid(gid_t gid) {
	errno = ENOSYS;
	return -1;
}

int setuid(gid_t gid) {
	errno = ENOSYS;
	return -1;
}

int getrusage(int who, struct rusage *r_usage) {
	errno = ENOSYS;
	return -1;
}
/*
int pselect(int nfds, fd_set *__restrict__ readfds,
		fd_set *__restrict__ writefds, fd_set *__restrict__ errorfds,
		const struct timespec *__restrict__ timeout,
		const sigset_t *__restrict__ sigmask) {
	errno = ENOSYS;
	-1;
}

int select(int nfds, fd_set *__restrict__ readfds,
		fd_set *__restrict__ writefds, fd_set *__restrict__ errorfds,
		struct timeval *__restrict__ timeout) {
	errno = ENOSYS;
	-1;
}
*/
struct passwd *getpwnam(const char *name) {
	errno = ENOSYS;
	return NULL;
}


struct passwd *getpwuid(uid_t uid) {
	errno = ENOSYS;
	return NULL;
}

struct group *getgrnam(const char *name) {
	errno = ENOSYS;
	return NULL;
}

struct group *getgrgid(gid_t gid) {
	errno = ENOSYS;
	return NULL;
}
pid_t setsid(void) {
	errno = ENOSYS;
	return -1;
}

int ftruncate(int fildes, off_t length) {
	errno = ENOSYS;
	return -1;
}

int accept(int socket, struct sockaddr* __restrict__ address, socklen_t* __restrict__ address_len) {
	errno = ENOSYS;
	return -1;
}

int getsockname(int socket, struct sockaddr* __restrict__ address,
       socklen_t* __restrict__ address_len) {
	errno = ENOSYS;
	return -1;
}

int setsockopt(int socket, int level, int option_name,
       const void *option_value, socklen_t option_len) {
	errno = ENOSYS;
	return -1;
}

int listen(int socket, int backlog) {
	return 0;
}
