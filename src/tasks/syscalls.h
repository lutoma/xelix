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
#include <tasks/wait.h>
#include <net/socket.h>
#include <fs/vfs.h>
#include <fs/pipe.h>
#include <time.h>

SYS_REDIR(open,			vfs_open,				(char*)syscall.params[0], syscall.params[1], syscall.task);
SYS_REDIR(read,			vfs_read,				syscall.params[0], (void*)syscall.params[1], syscall.params[2], syscall.task);
SYS_REDIR(write,		vfs_write,				syscall.params[0], (void*)syscall.params[1], syscall.params[2], syscall.task);
SYS_REDIR(stat,			vfs_stat,				syscall.params[0], (vfs_stat_t*)syscall.params[1], syscall.task);
SYS_REDIR(seek,			vfs_seek,				syscall.params[0], syscall.params[1], syscall.params[2], syscall.task);
SYS_REDIR(getdents,		vfs_getdents,			syscall.params[0], (void*)syscall.params[1], syscall.params[2], syscall.task);
SYS_REDIR(close,		vfs_close,				syscall.params[0], syscall.task);
SYS_REDIR(chmod,		vfs_chmod,				(char*)syscall.params[0], syscall.params[1], syscall.task);
SYS_REDIR(chown,		vfs_chown,				(char*)syscall.params[0], syscall.params[1], syscall.params[2], syscall.task);
SYS_REDIR(readlink,		vfs_readlink,			(const char*)syscall.params[0], (char*)syscall.params[1], syscall.params[2], syscall.task);
SYS_REDIR(mkdir,		vfs_mkdir,				(char*)syscall.params[0], syscall.params[1], syscall.task);
SYS_REDIR(rmdir,		vfs_rmdir,				(char*)syscall.params[0], syscall.task);
SYS_REDIR(unlink,		vfs_unlink,				(char*)syscall.params[0], syscall.task);
SYS_REDIR(pipe,			vfs_pipe,				(int*)syscall.params[0], syscall.task);
SYS_REDIR(access,		vfs_access,				(char*)syscall.params[0], syscall.params[1], syscall.task);
SYS_REDIR(link,			vfs_link,				(char*)syscall.params[0], (char*)syscall.params[1], syscall.task);
SYS_REDIR(utimes,		vfs_utimes,				(char*)syscall.params[0], (struct timeval*)syscall.params[1], syscall.task);
SYS_REDIR(sigprocmask,	task_sigprocmask,		syscall.task, syscall.params[0], (uint32_t*)syscall.params[1], (uint32_t*)syscall.params[2]);
SYS_REDIR(sigaction,	task_sigaction,			syscall.task, syscall.params[0], (struct sigaction*)syscall.params[1], (struct sigaction*)syscall.params[2]);
SYS_REDIR(gettimeofday,	time_get_timeval,		(struct timeval*)syscall.params[0]);
SYS_REDIR(chdir,		task_chdir,				syscall.task, (char*)syscall.params[0]);
SYS_REDIR(signal,		task_signal_syscall,	syscall.params[0], syscall.task, syscall.params[1], syscall.state);
SYS_REDIR(sbrk,			(uint32_t)task_sbrk,	syscall.task, syscall.params[1]);
SYS_REDIR(fcntl,		vfs_fcntl,				syscall.params[0], syscall.params[1], syscall.params[2], syscall.task);
SYS_REDIR(waitpid,		task_waitpid,			syscall.task, (int32_t)syscall.params[0], (int*)syscall.params[1], syscall.params[2]);
SYS_REDIR(fork,			task_fork,				syscall.task, syscall.state);
SYS_REDIR(exit,			task_exit,				syscall.task);
SYS_REDIR(execve,		task_execve,			syscall.task, (char*)syscall.params[0], (char**)syscall.params[1], (char**)syscall.params[2]);

#ifdef ENABLE_PICOTCP
SYS_REDIR(socket,		net_socket,				syscall.task, syscall.params[0], syscall.params[1], syscall.params[2]);
SYS_REDIR(bind,			net_bind,				syscall.task, syscall.params[0], (struct sockaddr*)syscall.params[1], syscall.params[2]);
SYS_REDIR(listen,		net_listen,				syscall.task, syscall.params[0], syscall.params[1]);
SYS_REDIR(accept,		net_accept,				syscall.task, syscall.params[0], (struct sockaddr*)syscall.params[1], syscall.params[2]);
SYS_REDIR(select,		net_select,				syscall.task, syscall.params[0], (fd_set*)syscall.params[1], (fd_set*)syscall.params[2]);
#else
SYS_DISABLED(socket);
SYS_DISABLED(bind);
SYS_DISABLED(listen);
SYS_DISABLED(accept);
SYS_DISABLED(select);
#endif


struct syscall_definition syscall_table[] = {
	{NULL, "", 0, 0, 0},
	{sys_exit, "exit", 0, 0, 0},
	{sys_read, "read", 0, SYSCALL_ARG_RESOLVE, 0},
	{sys_write, "write", 0, SYSCALL_ARG_RESOLVE, 0},
	{sys_access, "access", SYSCALL_ARG_RESOLVE, 0, 0},
	{sys_close, "close", 0, 0, 0},
	{sys_mkdir, "mkdir", SYSCALL_ARG_RESOLVE, 0, 0},
	{sys_sbrk, "sbrk", 0, 0, 0},
	{NULL, "symlink", 0, 0, 0},
	{NULL, "", 0, 0, 0},
	{sys_unlink, "unlink", SYSCALL_ARG_RESOLVE, 0, 0},
	{sys_chmod, "chmod", SYSCALL_ARG_RESOLVE, 0, 0},
	{sys_link, "link", SYSCALL_ARG_RESOLVE, SYSCALL_ARG_RESOLVE, 0},
	{sys_open, "open", SYSCALL_ARG_RESOLVE, 0, 0},
	{sys_stat, "stat", 0, SYSCALL_ARG_RESOLVE, 0},
	{sys_seek, "seek", 0, 0, 0},
	{sys_getdents, "getdents", 0, SYSCALL_ARG_RESOLVE, 0},
	{sys_chown, "chown", SYSCALL_ARG_RESOLVE, 0, 0},
	{sys_signal, "signal", 0, 0, 0},
	{sys_gettimeofday, "gettimeofday", SYSCALL_ARG_RESOLVE, 0, 0},
	{sys_chdir, "chdir", SYSCALL_ARG_RESOLVE, 0, 0},
	{sys_utimes, "utimes", SYSCALL_ARG_RESOLVE, 0, 0},
	{sys_fork, "fork", 0, 0, 0},
	{sys_rmdir, "rmdir", SYSCALL_ARG_RESOLVE, 0, 0},
	{sys_socket, "socket", 0, 0, 0},
	{sys_bind, "bind", 0, SYSCALL_ARG_RESOLVE, 0},
	{NULL, "", 0, 0, 0},
	{NULL, "", 0, 0, 0},
	{sys_pipe, "pipe", SYSCALL_ARG_RESOLVE, 0, 0},
	{sys_waitpid, "waitpid",
		0, SYSCALL_ARG_RESOLVE | SYSCALL_ARG_RESOLVE_NULL_OK, 0},
	{NULL, "", 0, 0, 0},
	{sys_readlink, "readlink", SYSCALL_ARG_RESOLVE, SYSCALL_ARG_RESOLVE, 0},
	{sys_execve, "execve",
		SYSCALL_ARG_RESOLVE, SYSCALL_ARG_RESOLVE, SYSCALL_ARG_RESOLVE},
	{sys_sigaction, "sigaction",
		0, SYSCALL_ARG_RESOLVE | SYSCALL_ARG_RESOLVE_NULL_OK,
		SYSCALL_ARG_RESOLVE | SYSCALL_ARG_RESOLVE_NULL_OK},
	{sys_sigprocmask, "sigprocmask",
		0, SYSCALL_ARG_RESOLVE | SYSCALL_ARG_RESOLVE_NULL_OK,
		SYSCALL_ARG_RESOLVE | SYSCALL_ARG_RESOLVE_NULL_OK},
	{NULL, "sigsuspend", 0, 0, 0},
	{sys_fcntl, "fcntl", 0, 0, 0},
	{sys_listen, "listen", 0, 0, 0},
	{sys_accept, "accept", 0, SYSCALL_ARG_RESOLVE, 0},
	{sys_select, "select", 0, SYSCALL_ARG_RESOLVE, SYSCALL_ARG_RESOLVE}
};
