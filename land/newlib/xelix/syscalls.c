/* Copyright © 2013-2020 Lukas Martini
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
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <libgen.h>
#include <stdio.h>
#include <stdarg.h>
#include <termios.h>
#include <signal.h>
#include <string.h>
#include <sgtty.h>
#include <limits.h>
#include <poll.h>
#include <utime.h>
#include <netdb.h>

/* Normally errno is defined as a macro that does reentrancy magic. However,
 * some of our syscalls (those prefixed with an underscore) get called from the
 * reentrant mappings in reent/ and syscall/, and those expect a plain errno.
 * For non-prefixed syscalls, we need to use the dynamic errno pointer.
 */
#undef errno
extern int errno;
#define syscall_pf(call, a1, a2, a3) __syscall(&errno, call, (uint32_t)a1, (uint32_t)a2, (uint32_t)a3)


void _exit(int return_code) {
	syscall_pf(1, return_code, 0, 0);
}

int _fork() {
	return syscall_pf(22, 0, 0, 0);
}

pid_t vfork(void) {
	return syscall(22, 0, 0, 0);
}

pid_t _getpid() {
	return (pid_t)_xelix_execdata->pid;
}

pid_t getppid(void) {
	return (pid_t)_xelix_execdata->ppid;
}

int _kill(int pid, int sig) {
	return syscall_pf(18, pid, sig, 0);
}

int _lseek(int file, int ptr, int dir) {
	return syscall_pf(15, file, ptr, dir);
}

int _open(const char* name, int flags, ...) {
	return syscall_pf(13, name, flags, 0);
}

int _close(int file) {
	return syscall_pf(5, file, 0, 0);
}

ssize_t _read(int file, char *buf, int len) {
	return syscall_pf(2, file, buf, len);
}

void* _sbrk(int incr) {
	return (void*)syscall_pf(7, incr, 0, 0);
}

void* mmap(void *addr, size_t len, int prot, int flags,
	int fildes, off_t off) {

	struct {
		void *addr;
		size_t len;
		int prot;
		int flags;
		int fildes;
		off_t off;
	} ctx = {addr, len, prot, flags, fildes, off};

	return (void*)syscall(8, &ctx, 0, 0);
}

int munmap(void *addr, size_t len) {
	return 0;
}

int poll(struct pollfd fds[], nfds_t nfds, int timeout) {
	return syscall(9, fds, nfds, timeout);
}

pid_t waitpid(pid_t pid, int* stat_loc, int options) {
	return syscall(29, pid, stat_loc, options);
}

int _wait(int* status) {
	// Call waitpid with pid -1, but redefine syscall as we need syscall_pf here.
	return syscall_pf(29, -1, status, 0);
}

int wait3(int* status) {
	return wait(status);
}

int _write(int file, char *buf, int len) {
	return syscall_pf(3, file, buf, len);
}

int chdir(const char *path) {
	return syscall(20, path, 0, 0);
}

int fchdir(int fd) {
	char path[PATH_MAX];
	if(fcntl(fd, F_GETPATH, path) == -1) {
		return -1;
	}

	return chdir(path);
}

int socket(int domain, int type, int protocol) {
	return syscall(24, domain, type, protocol);
}

int bind(int socket, const struct sockaddr *address, socklen_t address_len) {
	return syscall(25, socket, address, address_len);
}

struct _recvfrom_data {
	int sockfd;
	void* dest;
	size_t size;
	int flags;
	struct sockaddr* src_addr;
	socklen_t *addrlen;
};

ssize_t recvfrom(int socket, void *buffer, size_t length, int flags,
	struct sockaddr *address, socklen_t *address_len) {

	struct _recvfrom_data data = {
		.sockfd = socket,
		.dest = buffer,
		.size = length,
		.flags = flags,
		.src_addr = address,
		.addrlen = address_len,
	};
	return syscall(49, &data, sizeof(struct _recvfrom_data), 0);
}

ssize_t recv(int socket, void *buffer, size_t length, int flags) {
	return recvfrom(socket, buffer, length, flags, NULL, NULL);
}

ssize_t sendto(int socket, const void *message, size_t length, int flags,
	const struct sockaddr *dest_addr, socklen_t dest_len) {
	return syscall(3, socket, message, length);
}

ssize_t send(int socket, const void *buffer, size_t length, int flags) {
	return syscall(3, socket, buffer, length);
}

int _execve(char *name, char **argv, char **env) {
	return syscall(32, name, argv, env);
}

int fexecve(int fd, char * const argv[], char * const envp[]) {
	char path[PATH_MAX];
	if(fcntl(fd, F_GETPATH, path) == -1) {
		return -1;
	}

	return _execve(path, argv, envp);
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
	FILE* fp = fopen("/sys/version", "r");
	if(!fp) {
		return -1;
	}

	char sysname[50];
	char release[50];
	char version[300];
	char machine[50];
	int res = fscanf(fp, "%s %s \"%[^\"]\" %s\n", &sysname, &release, &version, &machine);
	fclose(fp);
	if(res != 4) {
		return -1;
	}

	strcpy(name->sysname, sysname);
	strcpy(name->release, release);
	strcpy(name->version, version);
	strcpy(name->machine, machine);
	gethostname(name->nodename, 300);
	return 0;
}

int _fstat(int file, struct stat* st) {
	return syscall_pf(14, file, st, 0);
}

int _stat(const char* name, struct stat *st) {
	return syscall_pf(43, name, st, 0);
}

int lstat(const char* name, struct stat *st) {
	return stat(name, st);
}

int _mkdir(const char *dir_path, mode_t mode) {
	return syscall_pf(6, dir_path, mode, 0);
}

int mkdir(const char *dir_path, mode_t mode) {
	return syscall(6, dir_path, mode, 0);
}

int _unlink(char *name) {
	return syscall_pf(10, name, 0, 0);
}

int chmod(const char *path, mode_t mode) {
	return syscall(11, path, mode, 0);
}

int fchmod(int fd, mode_t mode) {
	char path[PATH_MAX];
	if(fcntl(fd, F_GETPATH, path) == -1) {
		return -1;
	}

	return chmod(path, mode);
}


int chown(const char *path, uid_t owner, gid_t group) {
	return syscall(17, path, owner, group);
}

int fchown(int fd, uid_t owner, gid_t group) {
	char path[PATH_MAX];
	if(fcntl(fd, F_GETPATH, path) == -1) {
		return -1;
	}

	return chown(path, owner, group);
}

int access(const char *pathname, int mode) {
	return syscall(4, pathname, mode, 0);
}

int _gettimeofday(struct timeval* p, void* tz) {
	return syscall_pf(19, p, tz, 0);
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
	return syscall_pf(12, old, new, 0);
}

int readlink(const char *path, char *buf, size_t bufsize) {
	return syscall(31, path, buf, bufsize);
}

int sigaction(int sig, const struct sigaction* act, struct sigaction* oact) {
	return syscall(33, sig, act, oact);
}

int sigprocmask(int how, const sigset_t *set, sigset_t *oset) {
	return syscall(34, how, set, oset);
}

int sigsuspend(const sigset_t *sigmask) {
	return syscall(35, sigmask, 0, 0);
}

int getpagesize(void) {
	return 0x1000;
}

int pipe(int fildes[2]) {
	return syscall(28, fildes, 0, 0);
}

int _fcntl(int fildes, int cmd, ...) {
	va_list va;
	va_start(va, cmd);
	int r = syscall_pf(36, fildes, cmd, va_arg(va, int));
	va_end(va);
	return r;
}

int dup(int fildes) {
	return fcntl(fildes, F_DUPFD, 0);
}

int dup2(int fildes, int fildes2) {
	return syscall(44, fildes, fildes2, 0);
}


int listen(int socket, int backlog) {
	return syscall(37, socket, backlog, 0);
}

int accept(int socket, struct sockaddr* __restrict__ address, socklen_t* __restrict__ address_len) {
	return syscall(38, socket, address, address_len);
}

int getpeername(int socket, struct sockaddr *restrict address,
       socklen_t *restrict address_len) {

	return syscall(40, socket, address, address_len);
}

int getsockname(int socket, struct sockaddr* __restrict__ address,
       socklen_t* __restrict__ address_len) {

	return syscall(41, socket, address, address_len);
}

int _strace(void) {
	return syscall(45, 0, 0, 0);
}

int connect(int socket, const struct sockaddr *address, socklen_t address_len) {
	return syscall(48, socket, address, address_len);
}

int mount(const char *source, const char *target, const char *filesystemtype, unsigned long mountflags, const void *data) {
	return syscall(50, source, target, mountflags);
}

int umount2(const char *target, int flags) {
	return syscall(51, target, flags, 0);
}

int sched_yield(void) {
	asm volatile("int $0x31;");
	return 0;
}

int umount(const char *target) {
	umount2(target, 0);
}

char* realpath(const char *restrict file_name, char *restrict resolved_name) {
	char* buf = resolved_name ? resolved_name : malloc(PATH_MAX);
	if(syscall(52, file_name, buf, 0) < 0) {
		if(!resolved_name) {
			int sc_err = errno;
			free(buf);
			errno = sc_err;
		}
		return NULL;
	}

	return buf;
}

int ioctl(int fd, int request, ...) {
	va_list va;
	va_start(va, request);
	int r = syscall(26, fd, request, va_arg(va, uint32_t));
	va_end(va);
	return r;
}

int tcgetattr(int fd, struct termios *termios_p) {
	return ioctl(fd, TCGETS, termios_p);
}

int tcsetattr(int fd, int optional_actions, const struct termios* termios_p) {
	switch(optional_actions) {
		case TCSANOW:
			return ioctl(fd, TCSETS, termios_p);
		case TCSADRAIN:
			return ioctl(fd, TCSETSW, termios_p);
		case TCSAFLUSH:
			return ioctl(fd, TCSETSF, termios_p);
		default:
			errno = EINVAL;
			return -1;
	}
}

pid_t tcgetpgrp(int fd) {
	pid_t res;
	if(ioctl(fd, TIOCGPGRP, &res) < 0) {
		return -1;
	}
	return res;
}

int tcsetpgrp(int fd, pid_t pgid_id) {
	return ioctl(fd, TIOCSPGRP, &pgid_id);
}

int tcflush(int fd, int queue_selector) {
	return ioctl(fd, TCFLSH, queue_selector);
}

int tcsendbreak(int fd, int duration) {
	return ioctl(fd, TCSBRK, duration);
}

int tcflow(int fd, int action) {
	return ioctl(fd, TCXONC, action);
}

uid_t getuid(void) {
	return _xelix_execdata->uid;
}

uid_t geteuid(void) {
	return _xelix_execdata->euid;
}

uid_t getgid(void) {
	return _xelix_execdata->gid;
}

uid_t getegid(void) {
	return _xelix_execdata->egid;
}

int setuid(uid_t uid) {
	int r = syscall(42, 0, uid, 0);
	if(r >= 0) {
		_xelix_execdata->uid = uid;
	}
	return r;
}

int setgid(gid_t gid) {
	int r = syscall(42, 1, gid, 0);
	if(r >= 0) {
		_xelix_execdata->gid = gid;
	}
	return r;
}

char const* getprogname(void) {
	if(!_progname) {
		_progname = basename(_xelix_execdata->binary_path);
	}
	return _progname;
}

void setprogname(const char *name) {
	_progname = name;
}

const char *gai_strerror(int ecode) {
	switch(ecode) {
		case EAI_AGAIN: return "temporary failure in name resolution";
		case EAI_BADFLAGS: return "invalid value for ai_flags";
		case EAI_FAIL: return "non-recoverable failure in name resolution";
		case EAI_FAMILY: return "ai_family not supported.";
		case EAI_MEMORY: return "memory allocation failure";
		case EAI_NONAME: return "hostname or servname not provided,	or not known";
		case EAI_SERVICE: return "servname not supported for ai_socktype";
		case EAI_SOCKTYPE: return "ai_socktype not supported";
		case EAI_SYSTEM: return "system error returned in errno";
		default: return "unknown error";
	}
}

const char *hstrerror(int ecode) {
	switch(ecode) {
		case HOST_NOT_FOUND:
			return "The specified host is unknown.";
		case NO_DATA: /* Also matches NO_ADDRESS */
			return "The requested name is valid but does not have an IP address.";
		case NO_RECOVERY:
			return "A nonrecoverable name server error occurred.";
		case TRY_AGAIN:
			return "A temporary error occurred on an authoritative name server. Try again later.";
		default: return "unknown error";
	}
}

int ttyname_r(int fd, char *buf, size_t buflen) {
	return readlink("/dev/tty", buf, buflen) ? 0 : EBADF;
}


int usleep(useconds_t useconds) {
	struct timeval tv = { 0, useconds };
	return syscall(53, &tv, 0, 0);

}

unsigned sleep(unsigned seconds) {
	struct timeval tv = { seconds, 0 };
	return syscall(53, &tv, 0, 0);
}
