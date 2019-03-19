/* Copyright © 2013-2018 Lukas Martini *
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
#include <pthread.h>


clock_t _times(struct tms *buf) {
	errno = ENOSYS;
	return -1;
}

void _rewinddir(DIR* dd) {
	errno = ENOSYS;
	return;
}

void seekdir(DIR* dd, long int sd) {
	errno = ENOSYS;
	return;
}

speed_t cfgetispeed(const struct termios *termios_p) {
	errno = ENOSYS;
	return -1;
}

int cfsetispeed(struct termios *termios_p, speed_t speed) {
	errno = ENOSYS;
	return -1;
}

speed_t cfgetospeed(const struct termios *termios_p) {
	errno = ENOSYS;
	return -1;
}

int cfsetospeed(struct termios *termios_p, speed_t speed) {
	errno = ENOSYS;
	return -1;
}

char* _ttyname(int desc) {
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
	errno = ENOSYS;
	return -1;
}

int setgroups (int ngroups, const gid_t *grouplist) {
	errno = ENOSYS;
	return -1;
}

pid_t getpgrp(void) {
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
	errno = ENOSYS;
	return -1;
}

mode_t umask(mode_t cmask) {
	errno = ENOSYS;
	return -1;
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


pid_t setsid(void) {
	errno = ENOSYS;
	return -1;
}

int ftruncate(int fildes, off_t length) {
	errno = ENOSYS;
	return -1;
}

int setsockopt(int socket, int level, int option_name,
       const void *option_value, socklen_t option_len) {
	errno = ENOSYS;
	return -1;
}

int issetugid(void) {
	errno=ENOSYS;
	return 0;
}

// FIXME Should be a macro
int WCOREDUMP(int s) {
	return 0;
}

int poll(struct pollfd fds[], nfds_t nfds, int timeout) {
	errno = ENOSYS;
	return 0;
}

long sysconf(int name) {
	errno = ENOSYS;
	return 0;
}

int getrlimit(int resource, struct rlimit *rlim) {
	errno = ENOSYS;
	return -1;
}

int setrlimit(int resource, const struct rlimit *rlim) {
	errno = ENOSYS;
	return -1;
}

ssize_t getline(char **restrict lineptr, size_t *restrict n,
       FILE *restrict stream) {
	errno = ENOSYS;
	return -1;
}

int usleep(useconds_t useconds) {
	errno = ENOSYS;
	return -1;
}

unsigned sleep(unsigned seconds) {
	errno = ENOSYS;
	return -1;
}

int stime(time_t *t) {
	errno = ENOSYS;
	return -1;
}

long fpathconf(int fildes, int name) {
	errno = ENOSYS;
	return -1;
}
long pathconf(const char *path, int name) {
	errno = ENOSYS;
	return -1;
}

int fchmod(int fildes, mode_t mode) {
	errno = ENOSYS;
	return -1;
}

int lchmod(const char *path, mode_t mode) {
	errno = ENOSYS;
	return -1;
}

int fchown(int fd, uid_t owner, gid_t group) {
	errno = ENOSYS;
	return -1;
}

int lchown(const char *path, uid_t owner, gid_t group) {
	errno = ENOSYS;
	return -1;
}

int mknod(const char *path, mode_t mode, dev_t dev) {
	errno = ENOSYS;
	return -1;
}

int lutimes(const char *path, const struct timeval times[2]) {
	errno = ENOSYS;
	return -1;
}

int sched_yield(void) {
	errno = ENOSYS;
	return -1;
}

char *realpath(const char *restrict file_name,
       char *restrict resolved_name) {
	errno = ENOSYS;
	return NULL;
}

int fsync(int fildes) {
	errno = ENOSYS;
	return -1;
}

int getgrouplist(const char *user, gid_t group,
                        gid_t *groups, int *ngroups) {
	errno = ENOSYS;
	return -1;
}

int mkfifo(const char *path, mode_t mode) {
	errno = ENOSYS;
	return -1;
}

unsigned alarm(unsigned seconds) {
	errno = ENOSYS;
	return 0;
}

void flockfile(FILE *file) {}
int ftrylockfile(FILE *file) {
	errno = ENOSYS;
	return -1;
}
void funlockfile(FILE *file) {}


int fdatasync(int fildes) {
	errno = ENOSYS;
	return -1;
}

void err(int eval, const char *fmt, ...) {
	return 0;
}

int nanosleep(const struct timespec *rqtp, struct timespec *rmtp) {
	errno = ENOSYS;
	return -1;
}

int connect(int socket, const struct sockaddr *address,
	socklen_t address_len) {
	errno = ENOSYS;
	return -1;
}

struct servent *getservbyname(const char *name, const char *proto) {
	errno = ENOSYS;
	return NULL;
}

struct hostent *gethostbyname(const char *name) {
	errno = ENOSYS;
	return NULL;
}

int shutdown(int socket, int how) {
	errno = ENOSYS;
	return -1;
}

void freeaddrinfo(struct addrinfo *ai) {}

int getaddrinfo(const char *restrict nodename,
       const char *restrict servname,
       const struct addrinfo *restrict hints,
       struct addrinfo **restrict res) {
	errno = ENOSYS;
	return -1;
}

int getnameinfo(const struct sockaddr *restrict sa, socklen_t salen,
       char *restrict node, socklen_t nodelen, char *restrict service,
       socklen_t servicelen, int flags) {
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

int pthread_mutex_destroy(pthread_mutex_t *mutex) {
	return ENOSYS;
}

int pthread_mutex_init(pthread_mutex_t *restrict mutex,
       const pthread_mutexattr_t *restrict attr) {
	return ENOSYS;
}

int pthread_sigmask(int how, const sigset_t *restrict set,
       sigset_t *restrict oset) {
	return ENOSYS;
}

int pthread_mutex_lock(pthread_mutex_t *mutex) {
	return ENOSYS;
}

int pthread_mutex_trylock(pthread_mutex_t *mutex) {
	return ENOSYS;
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
	return ENOSYS;
}

int pthread_cond_init(pthread_cond_t *cond,
    const pthread_condattr_t *attr) {
	return ENOSYS;
}

int pthread_cond_destroy(pthread_cond_t *cond) {
	return ENOSYS;
}

int pthread_cond_signal(pthread_cond_t *cond) {
	return ENOSYS;
}

int pthread_cond_broadcast(pthread_cond_t *cond) {
	return ENOSYS;
}

int pthread_cond_wait(pthread_cond_t *cond, pthread_mutex_t *mutex) {
	return ENOSYS;
}

int pthread_cond_timedwait(pthread_cond_t *cond,
    pthread_mutex_t *mutex, const struct timespec *abstime) {
	return ENOSYS;
}

int pthread_join(pthread_t thread, void **value_ptr) {
	return ENOSYS;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
	void *(*start_routine) (void *), void *arg) {
	return ENOSYS;
}
