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

const struct syscall_definition syscall_table[] = {
	// 0
	{"", NULL, 0,
		SCA_UNUSED, SCA_UNUSED, SCA_UNUSED},

	// 1
	{"exit", (syscall_cb)task_exit, 0,
		SCA_INT, SCA_UNUSED, SCA_UNUSED},

	// 2
	{"read", (syscall_cb)vfs_read, SCF_TASKEND,
		SCA_INT, SCA_POINTER | SCA_TRANSLATE, SCA_INT},

	// 3
	{"write", (syscall_cb)vfs_write, SCF_TASKEND,
		SCA_INT, SCA_POINTER | SCA_TRANSLATE, SCA_INT},

	// 4
	{"access", (syscall_cb)vfs_access, SCF_TASKEND,
		SCA_STRING | SCA_TRANSLATE, SCA_INT, SCA_UNUSED},

	// 5
	{"close", (syscall_cb)vfs_close, SCF_TASKEND,
		SCA_INT, SCA_UNUSED, SCA_UNUSED},

	// 6
	{"mkdir", (syscall_cb)vfs_mkdir, SCF_TASKEND,
		SCA_STRING | SCA_TRANSLATE, SCA_INT, SCA_UNUSED},

	// 7
	{"sbrk", (syscall_cb)task_sbrk, 0,
		SCA_INT, SCA_INT, SCA_UNUSED},

	// 8
	{"symlink", NULL, 0,
		SCA_UNUSED, SCA_UNUSED, SCA_UNUSED},

	// 9
	{"poll", (syscall_cb)vfs_poll, SCF_TASKEND,
		SCA_POINTER | SCA_TRANSLATE, SCA_INT, SCA_INT},

	// 10
	{"unlink", (syscall_cb)vfs_unlink, SCF_TASKEND,
		SCA_STRING | SCA_TRANSLATE, SCA_UNUSED, SCA_UNUSED},

	// 11
	{"chmod", (syscall_cb)vfs_chmod, SCF_TASKEND,
		SCA_STRING | SCA_TRANSLATE, SCA_UNUSED, SCA_UNUSED},

	// 12
	{"link", (syscall_cb)vfs_link, SCF_TASKEND,
		SCA_STRING | SCA_TRANSLATE, SCA_STRING | SCA_TRANSLATE, SCA_UNUSED},

	// 13
	{"open", (syscall_cb)vfs_open, SCF_TASKEND,
		SCA_STRING | SCA_TRANSLATE, SCA_INT, SCA_UNUSED},

	// 14
	{"fstat", (syscall_cb)vfs_fstat, SCF_TASKEND,
		SCA_INT, SCA_POINTER | SCA_TRANSLATE, SCA_UNUSED},

	// 15
	{"seek", (syscall_cb)vfs_seek, SCF_TASKEND,
		SCA_INT, SCA_INT, SCA_INT},

	// 16
	{"getdents", (syscall_cb)vfs_getdents, SCF_TASKEND,
		SCA_INT, SCA_POINTER | SCA_TRANSLATE, SCA_INT},

	// 17
	{"chown", (syscall_cb)vfs_chown, SCF_TASKEND,
		SCA_STRING | SCA_TRANSLATE, SCA_INT, SCA_INT},

	// 18
	{"signal", (syscall_cb)task_signal_syscall, SCF_TASKEND,
		SCA_INT, SCA_INT, SCA_UNUSED},

	// 19
	{"gettimeofday", (syscall_cb)time_get_timeval, SCF_TASKEND,
		SCA_POINTER | SCA_TRANSLATE, SCA_UNUSED, SCA_UNUSED},

	// 20
	{"chdir", (syscall_cb)task_chdir, 0,
		SCA_STRING | SCA_TRANSLATE, SCA_UNUSED, SCA_UNUSED},

	// 21
	{"utimes", (syscall_cb)vfs_utimes, SCF_TASKEND,
		SCA_STRING | SCA_TRANSLATE, SCA_UNUSED, SCA_UNUSED},

	// 22
	{"fork", (syscall_cb)task_fork, SCF_STATE,
		SCA_UNUSED, SCA_UNUSED, SCA_UNUSED},

	// 23
	{"rmdir", (syscall_cb)vfs_rmdir,
		SCF_TASKEND, SCA_STRING | SCA_TRANSLATE, SCA_UNUSED, SCA_UNUSED},

#ifdef ENABLE_PICOTCP
	// 24
	{"socket", (syscall_cb)net_socket, 0,
		SCA_INT, SCA_INT, SCA_INT},

	// 25
	{"bind", (syscall_cb)net_bind, 0,
		SCA_INT, SCA_POINTER | SCA_TRANSLATE, SCA_INT},
#else
	// 24
	{"socket", NULL, 0,
		SCA_UNUSED, SCA_UNUSED, SCA_UNUSED},

	// 25
	{"bind", NULL, 0,
		SCA_UNUSED, SCA_UNUSED, SCA_UNUSED},
#endif

	// 26
	{"ioctl", (syscall_cb)vfs_ioctl, SCF_TASKEND,
		SCA_INT, SCA_INT, SCA_POINTER | SCA_TRANSLATE | SCA_NULLOK},

	// 27
	{"", NULL, 0,
		SCA_UNUSED, SCA_UNUSED, SCA_UNUSED},

	// 28
	{"pipe", (syscall_cb)vfs_pipe, 0,
		SCA_POINTER | SCA_TRANSLATE, SCA_UNUSED, SCA_UNUSED},

	// 29
	{"waitpid", (syscall_cb)task_waitpid, 0,
		SCA_INT, SCA_POINTER | SCA_TRANSLATE | SCA_NULLOK, SCA_INT},

	// 30
	{"", NULL, 0,
		SCA_UNUSED, SCA_UNUSED, SCA_UNUSED},

	// 31
	{"readlink", (syscall_cb)vfs_readlink, SCF_TASKEND,
		SCA_STRING | SCA_TRANSLATE, SCA_POINTER | SCA_TRANSLATE, SCA_INT},

	// 32
	{"execve", (syscall_cb)task_execve, 0,
		SCA_STRING | SCA_TRANSLATE, SCA_POINTER | SCA_TRANSLATE, SCA_POINTER | SCA_TRANSLATE},

	// 33
	{"sigaction", (syscall_cb)task_sigaction, 0,
		SCA_INT, SCA_POINTER | SCA_TRANSLATE | SCA_NULLOK,
		SCA_POINTER | SCA_TRANSLATE | SCA_NULLOK},

	// 34
	{"sigprocmask", (syscall_cb)task_sigprocmask, 0,
		SCA_INT, SCA_POINTER | SCA_TRANSLATE | SCA_NULLOK,
		SCA_POINTER | SCA_TRANSLATE | SCA_NULLOK},

	// 35
	{"sigsuspend", NULL, 0,
		SCA_UNUSED, SCA_UNUSED, SCA_UNUSED},

	// 36
	{"fcntl", (syscall_cb)vfs_fcntl, SCF_TASKEND,
		SCA_INT, SCA_INT, SCA_INT},

#ifdef ENABLE_PICOTCP
	// 37
	{"listen", (syscall_cb)net_listen, 0,
		SCA_INT, SCA_INT, SCA_UNUSED},

	// 38
	{"accept", (syscall_cb)net_accept, 0,
		SCA_INT, SCA_POINTER | SCA_TRANSLATE, SCA_POINTER},

	// 39
	{"select", (syscall_cb)net_select, 0,
		SCA_INT, SCA_POINTER | SCA_TRANSLATE, SCA_POINTER | SCA_TRANSLATE},

	// 40
	{"getpeername", (syscall_cb)net_getpeername, 0,
		SCA_INT, SCA_POINTER | SCA_TRANSLATE, SCA_POINTER | SCA_TRANSLATE},

	// 41
	{"getsockname", (syscall_cb)net_getsockname, 0,
		SCA_INT, SCA_POINTER | SCA_TRANSLATE, SCA_POINTER | SCA_TRANSLATE},
#else
	// 37
	{"socket", NULL, 0,
		SCA_UNUSED, SCA_UNUSED, SCA_UNUSED},

	// 38
	{"accept", NULL, 0,
		SCA_UNUSED, SCA_UNUSED, SCA_UNUSED},

	// 39
	{"select", NULL, 0,
		SCA_UNUSED, SCA_UNUSED, SCA_UNUSED},

	// 40
	{"getpeername", NULL, 0,
		SCA_UNUSED, SCA_UNUSED, SCA_UNUSED},

	// 41
	{"getsockname", NULL, 0,
		SCA_UNUSED, SCA_UNUSED, SCA_UNUSED},
#endif

	// 42
	{"setid", (syscall_cb)task_setid, 0,
		SCA_INT, SCA_INT, SCA_UNUSED},

	// 43
	{"stat", (syscall_cb)vfs_stat, SCF_TASKEND,
		SCA_STRING | SCA_TRANSLATE, SCA_POINTER | SCA_TRANSLATE, SCA_UNUSED},

	// 44
	{"dup2", (syscall_cb)vfs_dup2, SCF_TASKEND,
		SCA_INT, SCA_INT, SCA_UNUSED},

	// 45
	{"strace", (syscall_cb)task_strace, SCF_STATE,
		SCA_UNUSED, SCA_UNUSED, SCA_UNUSED},
};
