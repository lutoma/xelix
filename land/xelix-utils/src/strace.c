/* Copyright © 2019 Lukas Martini
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

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/xelix.h>
#include <sys/wait.h>
#include "util.h"


struct strace_data {
	uint32_t call;
	uint32_t result;
	uint32_t eno;
	uintptr_t args[3];
	char ptrdata[3][0x50];
};

struct saction {
	void (*handler)(struct strace_data* syscall, const char* data);
	char* data;
};

static void default_handler(struct strace_data* syscall, const char* fmt_string);
static void str_handler(struct strace_data* syscall, const char* fmt_string);
static void fd_handler(struct strace_data* syscall, const char* fmt_string);
static void open_handler(struct strace_data* syscall, const char* fmt_string);
static void sbrk_handler(struct strace_data* syscall, const char* fmt_string);

char seen_fds[200][PATH_MAX] = {
	"/dev/stdin",
	"/dev/stdout",
	"/dev/stderr",
};

static const struct saction const syscalls[] = {
	{NULL},
	{default_handler, "exit(code=%d)"},
	{fd_handler, "read(fd=%d%s, dest=%#x, size=%d)"},
	{fd_handler, "write(fd=%d%s, src=%#x, size=%d)"},
	{str_handler, "access(path=\"%s\", mode=%d)"},
	{fd_handler, "close(fd=%d%s)"},
	{str_handler, "mkdir(path=\"%s\", mode=%d)"},
	{sbrk_handler, NULL},
	{default_handler, "symlink()"},
	{default_handler, "poll()"},
	{str_handler, "unlink(path=\"%s\")"},
	{str_handler, "chmod(path=\"%s\", mode=%d)"},
	{default_handler, "link()"},
	{open_handler, NULL},
	{fd_handler, "fstat(fd=%d%s, stat=%#x)"},
	{default_handler, "seek()"},
	{fd_handler, "getdents(fd=%d%s, buf=%#x, size=%d)"},
	{str_handler, "chown(path=\"%s\", uid=%d, gid=%d)"},
	{default_handler, "signal(pid=%d, sig=%d)"},
	{default_handler, "gettimeofday()"},
	{str_handler, "chdir(path=\"%s\")"},
	{default_handler, "utimes()"},
	{default_handler, "fork()"},
	{str_handler, "rmdir(path=\"%s\")"},
	{default_handler, "socket()"},
	{default_handler, "bind()"},
	{fd_handler, "ioctl(fd=%d%s, req=%d, arg3=%d)"},
	{NULL},
	{default_handler, "pipe()"},
	{default_handler, "waitpid(pid=%d, wstat=%#x, options=%d)"},
	{NULL},
	{str_handler, "readlink(path=\"%s\", buf=%#x, size=%d)"},
	{str_handler, "execve(path=\"%s\", argv=%#x, env=%#x)"},
	{default_handler, "sigaction(sig=%d, act=%#x, oact=%#x)"},
	{default_handler, "sigprocmask(how=%d, set=%#x, oset=%#x)"},
	{default_handler, "sigsuspend()"},
	{fd_handler, "fcntl(fd=%d%s, cmd=%d, arg3=%d)"},
	{default_handler, "listen()"},
	{default_handler, "accept()"},
	{default_handler, "select()"},
	{default_handler, "getpeername()"},
	{default_handler, "getsockname()"},
	{default_handler, "setid()"},
	{default_handler, "stat()"},
	{default_handler, "dup2()"},
};

static const char* const usage[] = {
	"strace_data [options] prog [args]",
	NULL,
};

static void default_handler(struct strace_data* syscall, const char* fmt_string) {
	fprintf(stderr, fmt_string, syscall->args[0], syscall->args[1], syscall->args[2]);
	fprintf(stderr, " = %d", syscall->result);
	if(syscall->eno != 0) {
		fprintf(stderr, " (%d %s)", syscall->eno, strerror(syscall->eno));
	}
	fprintf(stderr, "\n");
}

static void str_handler(struct strace_data* syscall, const char* fmt_string) {
	fprintf(stderr, fmt_string, syscall->ptrdata[0], syscall->args[1], syscall->args[2]);
	fprintf(stderr, " = %d", syscall->result);

	if(syscall->eno != 0) {
		fprintf(stderr, " (%d %s)", syscall->eno, strerror(syscall->eno));
	}
	fprintf(stderr, "\n");
}

static void fd_handler(struct strace_data* syscall, const char* fmt_string) {
	char* file_path ;
	if(seen_fds[syscall->args[0]]) {
		asprintf(&file_path, " \"%s\"", seen_fds[syscall->args[0]]);
	} else {
		file_path = strdup("");
	}

	fprintf(stderr, fmt_string, syscall->args[0], file_path, syscall->args[1], syscall->args[2]);
	fprintf(stderr, " = %d", syscall->result);
	free(file_path);

	if(syscall->eno != 0) {
		fprintf(stderr, " (%d %s)", syscall->eno, strerror(syscall->eno));
	}
	fprintf(stderr, "\n");
}

static void sbrk_handler(struct strace_data* syscall, const char* fmt_string) {
	// Legacy: Size used to be args[1], while args[0] was unused
	int size = syscall->args[0] ? syscall->args[0] : syscall->args[1];
	fprintf(stderr, "sbrk(size=%d) = %#x", size, syscall->result);

	if(syscall->eno != 0) {
		fprintf(stderr, " (%d %s)", syscall->eno, strerror(syscall->eno));
	}
	fprintf(stderr, "\n");
}

static void open_handler(struct strace_data* syscall, const char* fmt_string) {
	if(syscall->result >= 0) {
		memcpy(seen_fds[syscall->result], syscall->ptrdata[0], 0x50);
	}

	fprintf(stderr, "open(path=\"%s\", flags=%x", syscall->ptrdata[0], syscall->args[1]);

	if(syscall->args[1] & O_RDONLY) {
		fprintf(stderr, " O_RDONLY");
	}
	if(syscall->args[1] & O_WRONLY) {
		fprintf(stderr, " O_WRONLY");
	}
	if(syscall->args[1] & O_RDWR) {
		fprintf(stderr, " O_RDWR");
	}
	if(syscall->args[1] & O_APPEND) {
		fprintf(stderr, " O_APPEND");
	}
	if(syscall->args[1] & O_CREAT) {
		fprintf(stderr, " O_CREAT");
	}
	if(syscall->args[1] & O_TRUNC) {
		fprintf(stderr, " O_TRUNC");
	}
	if(syscall->args[1] & O_EXCL) {
		fprintf(stderr, " O_EXCL");
	}
	if(syscall->args[1] & O_SYNC) {
		fprintf(stderr, " O_SYNC");
	}
	if(syscall->args[1] & O_NONBLOCK) {
		fprintf(stderr, " O_NONBLOCK");
	}

	fprintf(stderr, ") = %d", syscall->result);

	if(syscall->eno != 0) {
		fprintf(stderr, " (%d %s)", syscall->eno, strerror(syscall->eno));
	}
	fprintf(stderr, "\n");
}

int main(int argc, const char** argv) {
	if(argc < 2 || (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")))) {
		fprintf(stderr, "Usage: %s program [program arguments]\n\n", argv[0]);
		fprintf(stderr, "trace system calls and signals.\n\n");
		fprintf(stderr, "\t-h, --help\tshow this help message and exit\n\n");
		fprintf(stderr,
			"In the simplest case strace runs the specified command until it exits. It\n"
			"intercepts and records the system calls which are called by a process and the\n"
			"signals which are received by a process. The name of each system call, its\n"
			"arguments and its return value are printed on standard error.\n"
			"strace is part of xelix-utils. Please report bugs to <hello@lutoma.org>.\n");
		exit(EXIT_FAILURE);
	}

	sigset_t set;
	sigfillset(&set);
	sigdelset(&set, SIGINT);
	sigdelset(&set, SIGILL);
	sigdelset(&set, SIGFPE);
	sigdelset(&set, SIGSEGV);
	sigprocmask(SIG_SETMASK, &set, NULL);

	int trace_fd = _strace();
	if(trace_fd == -1) {
		perror("Could not start trace");
		exit(EXIT_FAILURE);
	}

	// Forked child
	if(trace_fd == 0) {
		execvp(argv[1], (char* const*)&argv[1]);
		perror("Could not spawn child process");
		exit(EXIT_FAILURE);
	}

	struct strace_data* syscall = calloc(1, sizeof(struct strace_data));
	while(true) {
		if(read(trace_fd, syscall, sizeof(struct strace_data)) != sizeof(struct strace_data)) {
			break;
		}

		if(syscall->call < (sizeof(syscalls) / sizeof(syscalls[0])) && syscalls[syscall->call].handler) {
			syscalls[syscall->call].handler(syscall, syscalls[syscall->call].data);
		}

		// exit syscall, manually end loop for now
		if(syscall->call == 1) {
			break;
		}
	}

	free(syscall);
	exit(EXIT_SUCCESS);
}
