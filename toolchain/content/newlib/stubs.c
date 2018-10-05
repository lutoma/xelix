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
#include <mntent.h>

int _link(char *old, char *new){
	//fprintf(stderr, "Warning: xelix newlib link() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int readlink(const char *path, char *buf, size_t bufsize) {
	//fprintf(stderr, "Warning: xelix newlib readlink() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int symlink(const char *path1, const char *path2) {
	//fprintf(stderr, "Warning: xelix newlib symlink() stub called.\n");

	errno = ENOSYS;
	return -1;
}

clock_t _times(struct tms *buf) {
	//fprintf(stderr, "Warning: xelix newlib times() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int _unlink(char *name) {
	//fprintf(stderr, "Warning: xelix newlib unlink() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int wait3(int* status) {
	//fprintf(stderr, "Warning: xelix newlib wait3() stub called.\n");

	errno = ENOSYS;
	return -1;
}

ssize_t pwrite(int fildes, const void *buf, size_t nbyte, off_t offset) {
	//fprintf(stderr, "Warning: xelix newlib pwrite() stub called.\n");

	errno = ENOSYS;
	return -1;
}

void rewinddir(DIR* dd) {
	//fprintf(stderr, "Warning: xelix newlib rewinddir() stub called.\n");

	errno = ENOSYS;
	return;
}

void seekdir(DIR* dd, long int sd) {
	//fprintf(stderr, "Warning: xelix newlib seekdir() stub called.\n");

	errno = ENOSYS;
	return;
}

speed_t cfgetospeed(const struct termios *termios_p) {
	//fprintf(stderr, "Warning: xelix newlib cfgetospeed() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int mkdir(const char *dir_path, mode_t mode) {
	//fprintf(stderr, "Warning: xelix newlib mkdir() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int access(const char *pathname, int mode) {
	//fprintf(stderr, "Warning: xelix newlib access() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int tcgetattr(int fd, struct termios *termios_p) {
	//fprintf(stderr, "Warning: xelix newlib tcgetattr() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int tcsetattr(int fd, int optional_actions, const struct termios* termios_p) {
	//fprintf(stderr, "Warning: xelix newlib tcsetattr() stub called.\n");

	errno = ENOSYS;
	return -1;
}

char* _ttyname(int desc) {
	//fprintf(stderr, "Warning: xelix newlib ttyname() stub called.\n");
	return "/dev/tty";
}

uid_t getuid(void) {
	//fprintf(stderr, "Warning: xelix newlib getuid() stub called.\n");

	errno = ENOSYS;
	return -1;
}

uid_t geteuid(void) {
	//fprintf(stderr, "Warning: xelix newlib geteuid() stub called.\n");

	errno = ENOSYS;
	return -1;
}

uid_t getgid(void) {
	//fprintf(stderr, "Warning: xelix newlib getgid() stub called.\n");

	errno = ENOSYS;
	return -1;
}

uid_t getegid(void) {
	//fprintf(stderr, "Warning: xelix newlib getegid() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int getgroups(int gidsetsize, gid_t grouplist[]) {
	//fprintf(stderr, "Warning: xelix newlib getgroups() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int setgroups(size_t size, const gid_t *list) {
	//fprintf(stderr, "Warning: xelix newlib setgroups() stub called.\n");

	errno = ENOSYS;
	return -1;
}

pid_t getpgrp(void) {
	//fprintf(stderr, "Warning: xelix newlib getpgrp() stub called.\n");

	// Simply return 1 as per POSIX getpgrp has no return code to indicate an
	// error
	return 1;
}

int sigprocmask(int how, const sigset_t *set, sigset_t *oset) {
	//fprintf(stderr, "Warning: xelix newlib sigprocmask() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int dup(int fildes) {
	//fprintf(stderr, "Warning: xelix newlib dup() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int dup2(int fildes, int fildes2) {
	//fprintf(stderr, "Warning: xelix newlib dup2() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int pipe(int fildes[2]) {
	//fprintf(stderr, "Warning: xelix newlib pipe() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int sigaction(int sig, const struct sigaction* act, struct sigaction* oact) {
	//fprintf(stderr, "Warning: xelix newlib sigaction() stub called.\n");

	errno = ENOSYS;
	return -1;
}

pid_t tcgetpgrp(int fildes) {
	//fprintf(stderr, "Warning: xelix newlib tcgetpgrp() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int setpgid(pid_t pid, pid_t pgid) {
	//fprintf(stderr, "Warning: xelix newlib setpgid() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int _fcntl(int fildes, int cmd, ...) {
	//fprintf(stderr, "Warning: xelix newlib fcntl() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int sigsuspend(const sigset_t *sigmask) {
	//fprintf(stderr, "Warning: xelix newlib sigsuspend() stub called.\n");

	errno = ENOSYS;
	return -1;
}

mode_t umask(mode_t cmask) {
	//fprintf(stderr, "Warning: xelix newlib umask() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int tcsetpgrp(int fildes, pid_t pgid_id) {
	//fprintf(stderr, "Warning: xelix newlib tcsetpgrp() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int gtty (int __fd, struct sgttyb *__params) {
	//fprintf(stderr, "Warning: xelix newlib gtty () stub called.\n");

	errno = ENOSYS;
	return 0;
}

/* Set the terminal parameters associated with FD to *PARAMS.  */
int stty (int __fd, __const struct sgttyb *__params) {
	//fprintf(stderr, "Warning: xelix newlib stty () stub called.\n");

	errno = ENOSYS;
	return 0;
}

int chroot(const char *path) {
	//fprintf(stderr, "Warning: xelix newlib chroot() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int setgid(gid_t gid) {
	//fprintf(stderr, "Warning: xelix newlib setgid() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int setuid(gid_t gid) {
	//fprintf(stderr, "Warning: xelix newlib setuid() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int getrusage(int who, struct rusage *r_usage) {
	//fprintf(stderr, "Warning: xelix newlib getrusage() stub called.\n");

	errno = ENOSYS;
	return -1;
}

/*
int pselect(int nfds, fd_set *__restrict__ readfds,
		fd_set *__restrict__ writefds, fd_set *__restrict__ errorfds,
		const struct timespec *__restrict__ timeout,
		const sigset_t *__restrict__ sigmask) {
	//fprintf(stderr, "Warning: xelix newlib getrusage() stub called.\n");

	errno = ENOSYS;
	-1;
}

int select(int nfds, fd_set *__restrict__ readfds,
		fd_set *__restrict__ writefds, fd_set *__restrict__ errorfds,
		struct timeval *__restrict__ timeout) {
	//fprintf(stderr, "Warning: xelix newlib getrusage() stub called.\n");

	errno = ENOSYS;
	-1;
}
*/


pid_t setsid(void) {
	//fprintf(stderr, "Warning: xelix newlib setsid() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int ftruncate(int fildes, off_t length) {
	//fprintf(stderr, "Warning: xelix newlib ftruncate() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int accept(int socket, struct sockaddr* __restrict__ address, socklen_t* __restrict__ address_len) {
	//fprintf(stderr, "Warning: xelix newlib accept() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int getsockname(int socket, struct sockaddr* __restrict__ address,
       socklen_t* __restrict__ address_len) {
	//fprintf(stderr, "Warning: xelix newlib accept() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int setsockopt(int socket, int level, int option_name,
       const void *option_value, socklen_t option_len) {
	//fprintf(stderr, "Warning: xelix newlib accept() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int listen(int socket, int backlog) {
	//fprintf(stderr, "Warning: xelix newlib listen() stub called.\n");

	return 0;
}

int issetugid(void) {
	//fprintf(stderr, "Warning: xelix newlib issetugid() stub called.\n");

	errno=ENOSYS;
	return 0;
}

// FIXME Should be a macro
int WCOREDUMP(int s) {
	//fprintf(stderr, "Warning: xelix newlib WCOREDUMP() stub called.\n");

	return 0;
}

int poll(struct pollfd fds[], nfds_t nfds, int timeout) {
	//fprintf(stderr, "Warning: xelix newlib poll() stub called.\n");

	errno = ENOSYS;
	return 0;
}

long sysconf(int name) {
	//fprintf(stderr, "Warning: xelix newlib sysconf() stub called.\n");

	errno = ENOSYS;
	return 0;
}

int getrlimit(int resource, struct rlimit *rlim) {
	//fprintf(stderr, "Warning: xelix newlib getrlimit() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int setrlimit(int resource, const struct rlimit *rlim) {
	//fprintf(stderr, "Warning: xelix newlib setrlimit() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int fchmod(int fildes, mode_t mode) {
	errno = ENOSYS;
	return -1;
}

int fchown(int fd, uid_t owner, gid_t group) {
	errno = ENOSYS;
	return -1;
}

pid_t vfork(void) {
	errno = ENOSYS;
	return -1;
}

int ioctl(int fildes, int request, void* rest) {
	errno = ENOSYS;
	return -1;
}

ssize_t getline(char **restrict lineptr, size_t *restrict n,
       FILE *restrict stream) {
	errno = ENOSYS;
	return -1;
}

struct group* getgrgid(gid_t gid) {
	errno = ENOSYS;
	return NULL;
}

// Should be macros
unsigned int major(dev_t dev) {
	return 0;
}
unsigned int minor(dev_t dev) {
	return dev;
}

int usleep(useconds_t useconds) {
	errno = ENOSYS;
	return -1;
}

unsigned sleep(unsigned seconds) {
	errno = ENOSYS;
	return -1;
}

int stime(const time_t *t) {
	errno = ENOSYS;
	return -1;
}

struct mntent *getmntent(FILE *stream) {
	errno = ENOSYS;
	return NULL;
}


long fpathconf(int fildes, int name) {
	errno = ENOSYS;
	return -1;
}
long pathconf(const char *path, int name) {
	errno = ENOSYS;
	return -1;
}

int tcflush(int fildes, int queue_selector) {
	errno = ENOSYS;
	return -1;
}

int select(int nfds, fd_set *readfds, fd_set *writefds,
	fd_set *exceptfds, struct timeval *timeout) {

	errno = ENOSYS;
	return -1;
}
