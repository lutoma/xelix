/* Copyright © 2013-2019 Lukas Martini
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
#include <syslog.h>

#ifdef NOISY_STUBS
	#define STUBWARN(cmd) fprintf(stderr, "xelix-newlib: " #cmd " stub called.\n");
#else
	#define STUBWARN(cmd)
#endif

#define STUB(rt, cmd, args, ret...) rt cmd args { \
	STUBWARN(cmd); \
	errno = ENOSYS; \
	return ret; \
}

int _isatty (int fd) {
	if(fd < 3) {
		return 1;
	}
	errno = ENOTTY;
	return 0;
}

STUB(clock_t, _times, (struct tms *buf), -1);
STUB(void, _rewinddir, (DIR* dd));
STUB(void, seekdir, (DIR* dd, long int sd));
STUB(speed_t, cfgetispeed, (const struct termios *termios_p), -1);
STUB(int, cfsetispeed, (struct termios *termios_p, speed_t speed), -1);
STUB(speed_t, cfgetospeed, (const struct termios *termios_p), -1);
STUB(int, cfsetospeed, (struct termios *termios_p, speed_t speed), -1);
STUB(int, getgroups, (int gidsetsize, gid_t grouplist[]), -1);
STUB(int, setgroups, (int ngroups, const gid_t *grouplist), -1);
STUB(pid_t, getpgrp, (void), -1);
STUB(int, setreuid, (uid_t ruid, uid_t euid), -1);
STUB(int, setregid, (gid_t rgid, gid_t egid), -1);
STUB(int, setpgid, (pid_t pid, pid_t pgid), -1);
STUB(mode_t, umask, (mode_t cmask), -1);
STUB(int, gtty, (int __fd, struct sgttyb *__params), -1);
STUB(int, stty, (int __fd, __const struct sgttyb *__params), -1);
STUB(int, chroot, (const char *path), -1);
STUB(int, getrusage, (int who, struct rusage *r_usage), -1);
STUB(pid_t, setsid, (void), -1);
STUB(int, ftruncate, (int fildes, off_t length), -1);
STUB(int, setsockopt, (int socket, int level, int option_name, const void *option_value, socklen_t option_len), -1);
STUB(int, issetugid, (void), -1);
STUB(long, sysconf, (int name), -1);
STUB(int, getrlimit, (int resource, struct rlimit *rlim), -1);
STUB(int, setrlimit, (int resource, const struct rlimit *rlim), -1);
STUB(ssize_t, getline, (char **restrict lineptr, size_t *restrict n, FILE *restrict stream), -1);
STUB(int, usleep, (useconds_t useconds), -1);
STUB(unsigned, sleep, (unsigned seconds), -1);
STUB(int, stime, (time_t *t), -1);
STUB(long, fpathconf, (int fildes, int name), -1);
STUB(long, pathconf, (const char *path, int name), -1);
STUB(int, fchmod, (int fildes, mode_t mode), -1);
STUB(int, lchmod, (const char *path, mode_t mode), -1);
STUB(int, fchown, (int fd, uid_t owner, gid_t group), -1);
STUB(int, lchown, (const char *path, uid_t owner, gid_t group), -1);
STUB(int, mknod, (const char *path, mode_t mode, dev_t dev), -1);
STUB(int, lutimes, (const char *path, const struct timeval times[2]), -1);
STUB(int, sched_yield, (void), -1);
STUB(char*, realpath, (const char *restrict file_name, char *restrict resolved_name), NULL);
STUB(int, fsync, (int fildes), -1);
STUB(int, getgrouplist, (const char *user, gid_t group, gid_t *groups, int *ngroups), -1);
STUB(int, mkfifo, (const char *path, mode_t mode), -1);
STUB(unsigned, alarm, (unsigned seconds), -1);
STUB(void, flockfile, (FILE *file));
STUB(int, ftrylockfile, (FILE *file), -1);
STUB(void, funlockfile, (FILE *file));
STUB(int, fdatasync, (int fildes), -1);
STUB(void, err, (int eval, const char *fmt, ...));
STUB(int, nanosleep, (const struct timespec *rqtp, struct timespec *rmtp), -1);
STUB(int, connect, (int socket, const struct sockaddr *address, socklen_t address_len), -1);
STUB(struct servent*, getservbyname, (const char *name, const char *proto), NULL);
STUB(struct hostent*, gethostbyname, (const char *name), NULL);
STUB(int, shutdown, (int socket, int how), -1);
STUB(void, freeaddrinfo, (struct addrinfo *ai));
STUB(int, pthread_mutex_destroy, (pthread_mutex_t *mutex), -1);
STUB(int, pthread_mutex_init, (pthread_mutex_t *restrict mutex, const pthread_mutexattr_t *restrict attr), -1);
STUB(int, pthread_sigmask, (int how, const sigset_t *restrict set, sigset_t *restrict oset), -1);
STUB(int, pthread_mutex_lock, (pthread_mutex_t *mutex), -1);
STUB(int, pthread_mutex_trylock, (pthread_mutex_t *mutex), -1);
STUB(int, pthread_mutex_unlock, (pthread_mutex_t *mutex), -1);
STUB(int, pthread_cond_init, (pthread_cond_t *cond, const pthread_condattr_t *attr), -1);
STUB(int, pthread_cond_destroy, (pthread_cond_t *cond), -1);
STUB(int, pthread_cond_signal, (pthread_cond_t *cond), -1);
STUB(int, pthread_cond_broadcast, (pthread_cond_t *cond), -1);
STUB(int, pthread_cond_wait, (pthread_cond_t *cond, pthread_mutex_t *mutex), -1);
STUB(int, pthread_cond_timedwait, (pthread_cond_t *cond, pthread_mutex_t *mutex, const struct timespec *abstime), -1);
STUB(int, pthread_join, (pthread_t thread, void **value_ptr), -1);
STUB(int, pthread_create, (pthread_t *thread, const pthread_attr_t *attr, void *(*start_routine) (void *), void *arg), -1);
STUB(int, killpg, (pid_t pid, int sig), -1);
STUB(void, closelog, (void));
STUB(void, openlog, (const char* ident, int logopt, int facility));
STUB(int, setlogmask, (int maskpri), -1);
STUB(void, syslog, (int prio, const char* fmt, ...));
STUB(int, initgroups, (const char *user, gid_t group), -1);
STUB(void, sync, (void));
STUB(int, getsockopt, (int sockfd, int level, int optname, void* optval, socklen_t* optlen), -1);
