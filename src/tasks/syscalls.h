#pragma once

/* Copyright © 2014-2019 Lukas Martini
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

#include <tasks/syscall.h>
#include <tasks/scheduler.h>
#include <tasks/signal.h>
#include <tasks/task.h>
#include <net/socket.h>
#include <fs/vfs.h>
#include <fs/pipe.h>
#include <time.h>

#define DEFINE_SYSCALL(name) extern uint32_t sys_ ## name (struct syscall syscall);
#define SYS_REDIR(name, args...) \
	static inline uint32_t sys_ ## name (struct syscall syscall) { \
		return args; \
	}

DEFINE_SYSCALL(exit);
DEFINE_SYSCALL(write);
DEFINE_SYSCALL(read);
DEFINE_SYSCALL(open);
DEFINE_SYSCALL(stat);
DEFINE_SYSCALL(seek);
DEFINE_SYSCALL(getdents);
DEFINE_SYSCALL(fork);
DEFINE_SYSCALL(socket_send);
DEFINE_SYSCALL(socket_recv);
DEFINE_SYSCALL(wait);
DEFINE_SYSCALL(audio_play);
DEFINE_SYSCALL(close);
DEFINE_SYSCALL(execve);

SYS_REDIR(chmod, vfs_chmod((char*)syscall.params[0], syscall.params[1], syscall.task));
SYS_REDIR(chown, vfs_chown((char*)syscall.params[0], syscall.params[1], syscall.params[2], syscall.task));
SYS_REDIR(readlink, vfs_readlink((const char*)syscall.params[0], (char*)syscall.params[1], syscall.params[2], syscall.task));
SYS_REDIR(mkdir, vfs_mkdir((char*)syscall.params[0], syscall.params[1], syscall.task));
SYS_REDIR(rmdir, vfs_rmdir((char*)syscall.params[0], syscall.task));
SYS_REDIR(unlink, vfs_unlink((char*)syscall.params[0], syscall.task));
SYS_REDIR(pipe, vfs_pipe((int*)syscall.params[0], syscall.task));
SYS_REDIR(access, vfs_access((char*)syscall.params[0], syscall.params[1], syscall.task));
SYS_REDIR(link, vfs_link((char*)syscall.params[0], (char*)syscall.params[1], syscall.task));
SYS_REDIR(utimes, vfs_utimes((char*)syscall.params[0], (struct timeval*)syscall.params[1], syscall.task));
SYS_REDIR(sigprocmask, task_sigprocmask(syscall.task, syscall.params[0], (uint32_t*)syscall.params[1], (uint32_t*)syscall.params[2]));
SYS_REDIR(sigaction, task_sigaction(syscall.task, syscall.params[0], (struct sigaction*)syscall.params[1], (struct sigaction*)syscall.params[2]));
SYS_REDIR(gettimeofday, time_get_timeval((struct timeval*)syscall.params[0]));
SYS_REDIR(chdir, task_chdir(syscall.task, (char*)syscall.params[0]));
SYS_REDIR(signal, task_signal_syscall(syscall.params[0], syscall.task, syscall.params[1], syscall.state));
SYS_REDIR(sbrk, (uint32_t)task_sbrk(syscall.task, syscall.params[1]));
SYS_REDIR(socket, net_socket(syscall.task, syscall.params[0], syscall.params[1], syscall.params[2]));
SYS_REDIR(bind, net_bind(syscall.task, syscall.params[0], (struct sockaddr*)syscall.params[1], syscall.params[2]));
SYS_REDIR(listen, net_listen(syscall.task, syscall.params[0], syscall.params[1]));
SYS_REDIR(accept, net_accept(syscall.task, syscall.params[0], (struct sockaddr*)syscall.params[1], syscall.params[2]));
SYS_REDIR(select, net_select(syscall.task, syscall.params[0], (fd_set*)syscall.params[1], (fd_set*)syscall.params[2]));
SYS_REDIR(fcntl, vfs_fcntl(syscall.params[0], syscall.params[1], syscall.params[2], syscall.task));

#define SYSCALL_ARG_RESOLVE 1
#define SYSCALL_ARG_RESOLVE_NULL_OK 2

struct syscall_definition {
	syscall_t handler;
	char name[50];

	uint8_t arg0;
	uint8_t arg1;
	uint8_t arg2;
};

struct syscall_definition syscall_table[] = {
	{ // 0
		.handler = NULL,
		.name = "",
		.arg0 = 0,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 1
		.handler = sys_exit,
		.name = "exit",
		.arg0 = 0,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 2
		.handler = sys_read,
		.name = "read",
		.arg0 = 0,
		.arg1 = SYSCALL_ARG_RESOLVE,
		.arg2 = 0,
	},
	{ // 3
		.handler = sys_write,
		.name = "write",
		.arg0 = 0,
		.arg1 = SYSCALL_ARG_RESOLVE,
		.arg2 = 0,
	},
	{ // 4
		.handler = sys_access,
		.name = "access",
		.arg0 = SYSCALL_ARG_RESOLVE,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 5
		.handler = sys_close,
		.name = "close",
		.arg0 = 0,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 6
		.handler = sys_mkdir,
		.name = "mkdir",
		.arg0 = SYSCALL_ARG_RESOLVE,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 7
		.handler = sys_sbrk,
		.name = "sbrk",
		.arg0 = 0,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 8
		.handler = NULL,
		.name = "symlink",
		.arg0 = 0,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 9
		.handler = NULL,
		.name = "",
		.arg0 = 0,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 10
		.handler = sys_unlink,
		.name = "unlink",
		.arg0 = SYSCALL_ARG_RESOLVE,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 11
		.handler = sys_chmod,
		.name = "chmod",
		.arg0 = SYSCALL_ARG_RESOLVE,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 12
		.handler = sys_link,
		.name = "link",
		.arg0 = SYSCALL_ARG_RESOLVE,
		.arg1 = SYSCALL_ARG_RESOLVE,
		.arg2 = 0,
	},
	{ // 13
		.handler = sys_open,
		.name = "open",
		.arg0 = SYSCALL_ARG_RESOLVE,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 14
		.handler = sys_stat,
		.name = "stat",
		.arg0 = 0,
		.arg1 = SYSCALL_ARG_RESOLVE,
		.arg2 = 0,
	},
	{ // 15
		.handler = sys_seek,
		.name = "seek",
		.arg0 = 0,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 16
		.handler = sys_getdents,
		.name = "getdents",
		.arg0 = 0,
		.arg1 = SYSCALL_ARG_RESOLVE,
		.arg2 = 0,
	},
	{ // 17
		.handler = sys_chown,
		.name = "chown",
		.arg0 = SYSCALL_ARG_RESOLVE,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 18
		.handler = sys_signal,
		.name = "signal",
		.arg0 = 0,
		.arg1 = 0,
		.arg2 = 0
	},
	{ // 19
		.handler = sys_gettimeofday,
		.name = "gettimeofday",
		.arg0 = SYSCALL_ARG_RESOLVE,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 20
		.handler = sys_chdir,
		.name = "chdir",
		.arg0 = SYSCALL_ARG_RESOLVE,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 21
		.handler = sys_utimes,
		.name = "utimes",
		.arg0 = SYSCALL_ARG_RESOLVE,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 22
		.handler = sys_fork,
		.name = "fork",
		.arg0 = 0,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 23
		.handler = sys_rmdir,
		.name = "rmdir",
		.arg0 = SYSCALL_ARG_RESOLVE,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 24
		.handler = sys_socket,
		.name = "socket",
		.arg0 = 0,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 25
		.handler = sys_bind,
		.name = "bind",
		.arg0 = 0,
		.arg1 = SYSCALL_ARG_RESOLVE,
		.arg2 = 0
	},
	{ // 26
		.handler = sys_socket_send,
		.name = "socket_send",
		.arg0 = 0,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 27
		.handler = sys_socket_recv,
		.name = "socket_recv",
		.arg0 = 0,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 28
		.handler = sys_pipe,
		.name = "pipe",
		.arg0 = SYSCALL_ARG_RESOLVE,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 29
		.handler = sys_wait,
		.name = "wait",
		.arg0 = 0,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 30
		.handler = sys_audio_play,
		.name = "audio_play",
		.arg0 = SYSCALL_ARG_RESOLVE,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 31
		.handler = sys_readlink,
		.name = "readlink",
		.arg0 = SYSCALL_ARG_RESOLVE,
		.arg1 = SYSCALL_ARG_RESOLVE,
		.arg2 = 0,
	},
	{ // 32
		.handler = sys_execve,
		.name = "execve",
		.arg0 = SYSCALL_ARG_RESOLVE,
		.arg1 = SYSCALL_ARG_RESOLVE,
		.arg2 = SYSCALL_ARG_RESOLVE,
	},
	{ // 33
		.handler = sys_sigaction,
		.name = "sigaction",
		.arg0 = 0,
		.arg1 = SYSCALL_ARG_RESOLVE | SYSCALL_ARG_RESOLVE_NULL_OK,
		.arg2 = SYSCALL_ARG_RESOLVE | SYSCALL_ARG_RESOLVE_NULL_OK,
	},
	{ // 34
		.handler = sys_sigprocmask,
		.name = "sigprocmask",
		.arg0 = 0,
		.arg1 = SYSCALL_ARG_RESOLVE | SYSCALL_ARG_RESOLVE_NULL_OK,
		.arg2 = SYSCALL_ARG_RESOLVE | SYSCALL_ARG_RESOLVE_NULL_OK,
	},
	{ // 35
		.handler = NULL,
		.name = "sigsuspend",
		.arg0 = 0,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 36
		.handler = sys_fcntl,
		.name = "fcntl",
		.arg0 = 0,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 37
		.handler = sys_listen,
		.name = "listen",
		.arg0 = 0,
		.arg1 = 0,
		.arg2 = 0,
	},
	{ // 38
		.handler = sys_accept,
		.name = "accept",
		.arg0 = 0,
		.arg1 = SYSCALL_ARG_RESOLVE,
		.arg2 = 0,
	},
	{ // 39
		.handler = sys_select,
		.name = "select",
		.arg0 = 0,
		.arg1 = SYSCALL_ARG_RESOLVE,
		.arg2 = SYSCALL_ARG_RESOLVE,
	}
};
