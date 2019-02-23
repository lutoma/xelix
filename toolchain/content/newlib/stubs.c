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
#include <sys/dirent.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/errno.h>
#include <sys/stat.h>
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


clock_t _times(struct tms *buf) {
	fprintf(stderr, "Warning: xelix newlib times() stub called.\n");

	errno = ENOSYS;
	return -1;
}

void _rewinddir(DIR* dd) {
	fprintf(stderr, "Warning: xelix newlib rewinddir() stub called.\n");

	errno = ENOSYS;
	return;
}

void seekdir(DIR* dd, long int sd) {
	fprintf(stderr, "Warning: xelix newlib seekdir() stub called.\n");

	errno = ENOSYS;
	return;
}

speed_t cfgetospeed(const struct termios *termios_p) {
	fprintf(stderr, "Warning: xelix newlib cfgetospeed() stub called.\n");

	errno = ENOSYS;
	return -1;
}

char* _ttyname(int desc) {
	fprintf(stderr, "Warning: xelix newlib ttyname() stub called.\n");
	return "/dev/tty";
}

uid_t getuid(void) {
	return 0;
}

uid_t geteuid(void) {
	return 0;
}

uid_t getgid(void) {
	return 0;
}

uid_t getegid(void) {
	return 0;
}

int getgroups(int gidsetsize, gid_t grouplist[]) {
	fprintf(stderr, "Warning: xelix newlib getgroups() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int setgroups(size_t size, const gid_t *list) {
	fprintf(stderr, "Warning: xelix newlib setgroups() stub called.\n");

	errno = ENOSYS;
	return -1;
}

pid_t getpgrp(void) {
	fprintf(stderr, "Warning: xelix newlib getpgrp() stub called.\n");

	// Simply return 1 as per POSIX getpgrp has no return code to indicate an
	// error
	return 1;
}

int setreuid(uid_t ruid, uid_t euid) {
	errno = ENOSYS;
	return -1;
}

int setregid(gid_t rgid, gid_t egid) {
	errno = ENOSYS;
	return -1;
}

int setpgid(pid_t pid, pid_t pgid) {
	fprintf(stderr, "Warning: xelix newlib setpgid() stub called.\n");

	errno = ENOSYS;
	return -1;
}

mode_t umask(mode_t cmask) {
	fprintf(stderr, "Warning: xelix newlib umask() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int gtty (int __fd, struct sgttyb *__params) {
	fprintf(stderr, "Warning: xelix newlib gtty () stub called.\n");

	errno = ENOSYS;
	return 0;
}

/* Set the terminal parameters associated with FD to *PARAMS.  */
int stty (int __fd, __const struct sgttyb *__params) {
	fprintf(stderr, "Warning: xelix newlib stty () stub called.\n");

	errno = ENOSYS;
	return 0;
}

int chroot(const char *path) {
	fprintf(stderr, "Warning: xelix newlib chroot() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int setgid(gid_t gid) {
	fprintf(stderr, "Warning: xelix newlib setgid() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int setuid(gid_t gid) {
	fprintf(stderr, "Warning: xelix newlib setuid() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int getrusage(int who, struct rusage *r_usage) {
	fprintf(stderr, "Warning: xelix newlib getrusage() stub called.\n");

	errno = ENOSYS;
	return -1;
}

/*
int pselect(int nfds, fd_set *__restrict__ readfds,
		fd_set *__restrict__ writefds, fd_set *__restrict__ errorfds,
		const struct timespec *__restrict__ timeout,
		const sigset_t *__restrict__ sigmask) {
	fprintf(stderr, "Warning: xelix newlib getrusage() stub called.\n");

	errno = ENOSYS;
	-1;
}

int select(int nfds, fd_set *__restrict__ readfds,
		fd_set *__restrict__ writefds, fd_set *__restrict__ errorfds,
		struct timeval *__restrict__ timeout) {
	fprintf(stderr, "Warning: xelix newlib getrusage() stub called.\n");

	errno = ENOSYS;
	-1;
}
*/


pid_t setsid(void) {
	fprintf(stderr, "Warning: xelix newlib setsid() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int ftruncate(int fildes, off_t length) {
	fprintf(stderr, "Warning: xelix newlib ftruncate() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int getsockname(int socket, struct sockaddr* __restrict__ address,
       socklen_t* __restrict__ address_len) {
	fprintf(stderr, "Warning: xelix newlib getsockname() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int setsockopt(int socket, int level, int option_name,
       const void *option_value, socklen_t option_len) {
	fprintf(stderr, "Warning: xelix newlib setsockopt() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int issetugid(void) {
	fprintf(stderr, "Warning: xelix newlib issetugid() stub called.\n");

	errno=ENOSYS;
	return 0;
}

// FIXME Should be a macro
int WCOREDUMP(int s) {
	fprintf(stderr, "Warning: xelix newlib WCOREDUMP() stub called.\n");

	return 0;
}

int poll(struct pollfd fds[], nfds_t nfds, int timeout) {
	fprintf(stderr, "Warning: xelix newlib poll() stub called.\n");

	errno = ENOSYS;
	return 0;
}

long sysconf(int name) {
	fprintf(stderr, "Warning: xelix newlib sysconf() stub called.\n");

	errno = ENOSYS;
	return 0;
}

int getrlimit(int resource, struct rlimit *rlim) {
	fprintf(stderr, "Warning: xelix newlib getrlimit() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int setrlimit(int resource, const struct rlimit *rlim) {
	fprintf(stderr, "Warning: xelix newlib setrlimit() stub called.\n");

	errno = ENOSYS;
	return -1;
}

pid_t vfork(void) {
	fprintf(stderr, "Warning: xelix newlib vfork() stub called.\n");
	errno = ENOSYS;
	return -1;
}

ssize_t getline(char **restrict lineptr, size_t *restrict n,
       FILE *restrict stream) {
	fprintf(stderr, "Warning: xelix newlib getline() stub called.\n");
	errno = ENOSYS;
	return -1;
}

int usleep(useconds_t useconds) {
	fprintf(stderr, "Warning: xelix newlib usleep() stub called.\n");
	errno = ENOSYS;
	return -1;
}

unsigned sleep(unsigned seconds) {
	fprintf(stderr, "Warning: xelix newlib sleep() stub called.\n");
	errno = ENOSYS;
	return -1;
}

int stime(time_t *t) {
	fprintf(stderr, "Warning: xelix newlib stime() stub called.\n");
	errno = ENOSYS;
	return -1;
}

long fpathconf(int fildes, int name) {
	fprintf(stderr, "Warning: xelix newlib fpathconf() stub called.\n");
	errno = ENOSYS;
	return -1;
}
long pathconf(const char *path, int name) {
	fprintf(stderr, "Warning: xelix newlib pathconf() stub called.\n");
	errno = ENOSYS;
	return -1;
}

int fchmod(int fildes, mode_t mode) {
	fprintf(stderr, "Warning: xelix newlib fchmod() stub called.\n");
	errno = ENOSYS;
	return -1;
}

int lchmod(const char *path, mode_t mode) {
	fprintf(stderr, "Warning: xelix newlib lchmod() stub called.\n");
	errno = ENOSYS;
	return -1;
}

int fchown(int fd, uid_t owner, gid_t group) {
	fprintf(stderr, "Warning: xelix newlib fchown() stub called.\n");
	errno = ENOSYS;
	return -1;
}

int lchown(const char *path, uid_t owner, gid_t group) {
	fprintf(stderr, "Warning: xelix newlib lchown() stub called.\n");
	errno = ENOSYS;
	return -1;
}

int mknod(const char *path, mode_t mode, dev_t dev) {
	fprintf(stderr, "Warning: xelix newlib mknod() stub called.\n");
 	errno = ENOSYS;
	return -1;
}

int lutimes(const char *path, const struct timeval times[2]) {
	fprintf(stderr, "Warning: xelix newlib lutimes() stub called.\n");
 	errno = ENOSYS;
	return -1;
}

int sched_yield(void) {
	fprintf(stderr, "Warning: xelix newlib sched_yield() stub called.\n");
 	errno = ENOSYS;
	return -1;
}

char *realpath(const char *restrict file_name,
       char *restrict resolved_name) {
	fprintf(stderr, "Warning: xelix newlib realpath() stub called.\n");
	errno = ENOSYS;
	return NULL;
}

int fsync(int fildes) {
	fprintf(stderr, "Warning: xelix newlib fsync() stub called.\n");
	errno = ENOSYS;
	return -1;
}

int getgrouplist(const char *user, gid_t group,
                        gid_t *groups, int *ngroups) {
	fprintf(stderr, "Warning: xelix newlib getgrouplist() stub called.\n");

	errno = ENOSYS;
	return -1;
}

int mkfifo(const char *path, mode_t mode) {
	fprintf(stderr, "Warning: xelix newlib mkfifo() stub called.\n");
	errno = ENOSYS;
	return -1;
}

unsigned alarm(unsigned seconds) {
	fprintf(stderr, "Warning: xelix newlib alarm() stub called.\n");
	errno = ENOSYS;
	return 0;
}

void flockfile(FILE *file) {}
int ftrylockfile(FILE *file) {
	fprintf(stderr, "Warning: xelix newlib ftrylockfile() stub called.\n");
	return -1;
	errno = ENOSYS;
}
void funlockfile(FILE *file) {}


int fdatasync(int fildes) {
	fprintf(stderr, "Warning: xelix newlib fdatasync() stub called.\n");
	errno = ENOSYS;
	return -1;
}

void err(int eval, const char *fmt, ...) {
	fprintf(stderr, "Warning: xelix newlib err() stub called.\n");
	return 0;
}

int nanosleep(const struct timespec *rqtp, struct timespec *rmtp) {
	fprintf(stderr, "Warning: xelix newlib nanosleep() stub called.\n");
	errno = ENOSYS;
	return -1;
}

int connect(int socket, const struct sockaddr *address,
	socklen_t address_len) {

	fprintf(stderr, "Warning: xelix newlib connect() stub called.\n");
	errno = ENOSYS;
	return -1;
}

int getpeername(int socket, struct sockaddr *restrict address,
       socklen_t *restrict address_len) {

	fprintf(stderr, "Warning: xelix newlib getpeername() stub called.\n");
	errno = ENOSYS;
	return -1;
}

struct servent *getservbyname(const char *name, const char *proto) {
	fprintf(stderr, "Warning: xelix newlib getservbyname() stub called.\n");
	errno = ENOSYS;
	return NULL;
}

struct hostent *gethostbyname(const char *name) {
	fprintf(stderr, "Warning: xelix newlib gethostbyname() stub called.\n");
	errno = ENOSYS;
	return NULL;
}

int shutdown(int socket, int how) {
	fprintf(stderr, "Warning: xelix newlib shutdown() stub called.\n");
	errno = ENOSYS;
	return -1;
}

void freeaddrinfo(struct addrinfo *ai) {
	fprintf(stderr, "Warning: xelix newlib freeaddrinfo() stub called.\n");
}

int getaddrinfo(const char *restrict nodename,
       const char *restrict servname,
       const struct addrinfo *restrict hints,
       struct addrinfo **restrict res) {
	fprintf(stderr, "Warning: xelix newlib getaddrinfo() stub called.\n");
	errno = ENOSYS;
	return -1;
}

int getnameinfo(const struct sockaddr *restrict sa, socklen_t salen,
       char *restrict node, socklen_t nodelen, char *restrict service,
       socklen_t servicelen, int flags) {
	fprintf(stderr, "Warning: xelix newlib getnameinfo() stub called.\n");
	errno = ENOSYS;
	return -1;
}

int _isatty (int fd) {
	if(fd < 3) {
		return 1;
	}
	errno = ENOTTY;
	return 0;
}
