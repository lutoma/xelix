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

/* Syscall definitions
 *
 * Entry format:
 *	{"name", callback, flags,
 *		arg0_flags, arg1_flags, arg2_flags, ptr_size},
 *
 * The name is purely cosmetic and only used if syscall debugging is enabled.
 *
 * Callback signature:
 *  typedef uint32_t (*syscall_cb)(task_t* task, [isf_t* state],
 * 		syscall arguments..)
 *
 * Possible flags are SCF_TASKEND (calls the callback with task as the last
 * argument instead of the first, legacy) and SCF_STATE (Also passes on the isf
 * state to the callback, cannot be used with SCF_TASKEND).
 *
 * In addition, each argument has an individual flags field. If the flags field
 * for an argument is 0, the argument is ignored (and the callback will be
 * called without it). Available flags for these:
 *
 * SCA_INT Argument is a normal integer. Will be passed to the callback 1:1,
 * and output as %d during syscall debugging.
 * SCA_POINTER Argument is a buffer of some sort. The address will be
 * checked to make sure it's inside the task's writable memory, then gets
 * translated to kernel memory. Output as %#x.
 * SCA_STRING Same as SCA_POINTER, except it's output as %s.
 * SCA_NULLOK If the type is SCA_POINTER or SCA_STRING, mark a value of NULL/0
 * as acceptable.
 */

const struct syscall_definition syscall_table[] = {
	// 0
	{"", NULL, 0,
		0, 0, 0, 0},

	// 1
	{"exit", (syscall_cb)task_exit, 0,
		SCA_INT, 0, 0, 0},

	// 2
	{"read", (syscall_cb)vfs_read, SCF_TASKEND,
		SCA_INT, SCA_POINTER | SCA_SIZE_IN_2, SCA_INT, 0},

	// 3
	{"write", (syscall_cb)vfs_write, SCF_TASKEND,
		SCA_INT, SCA_POINTER | SCA_SIZE_IN_2, SCA_INT, 0},

	// 4
	{"access", (syscall_cb)vfs_access, SCF_TASKEND,
		SCA_STRING, SCA_INT, 0, 0},

	// 5
	{"close", (syscall_cb)vfs_close, SCF_TASKEND,
		SCA_INT, 0, 0, 0},

	// 6
	{"mkdir", (syscall_cb)vfs_mkdir, SCF_TASKEND,
		SCA_STRING, SCA_INT, 0, 0},

	// 7
	{"sbrk", (syscall_cb)task_sbrk, 0,
		SCA_INT, SCA_INT, 0, 0},

	// 8
	{"symlink", NULL, 0,
		0, 0, 0, 0},

	// 9
	{"poll", (syscall_cb)vfs_poll, SCF_TASKEND,
		SCA_POINTER | SCA_SIZE_IN_1, SCA_INT, SCA_INT, sizeof(struct pollfd)},

	// 10
	{"unlink", (syscall_cb)vfs_unlink, SCF_TASKEND,
		SCA_STRING, 0, 0, 0},

	// 11
	{"chmod", (syscall_cb)vfs_chmod, SCF_TASKEND,
		SCA_STRING, 0, 0, 0},

	// 12
	{"link", (syscall_cb)vfs_link, SCF_TASKEND,
		SCA_STRING, SCA_STRING, 0, 0},

	// 13
	{"open", (syscall_cb)vfs_open, SCF_TASKEND,
		SCA_STRING, SCA_INT, 0, 0},

	// 14
	{"fstat", (syscall_cb)vfs_fstat, SCF_TASKEND,
		SCA_INT, SCA_POINTER, 0, sizeof(vfs_stat_t)},

	// 15
	{"seek", (syscall_cb)vfs_seek, SCF_TASKEND,
		SCA_INT, SCA_INT, SCA_INT, 0},

	// 16
	{"getdents", (syscall_cb)vfs_getdents, SCF_TASKEND,
		SCA_INT, SCA_POINTER | SCA_SIZE_IN_2, SCA_INT, 0},

	// 17
	{"chown", (syscall_cb)vfs_chown, SCF_TASKEND,
		SCA_STRING, SCA_INT, SCA_INT, 0},

	// 18
	{"signal", (syscall_cb)task_signal_syscall, SCF_TASKEND,
		SCA_INT, SCA_INT, 0, 0},

	// 19
	{"gettimeofday", (syscall_cb)time_get_timeval, SCF_TASKEND,
		SCA_POINTER, 0, 0, sizeof(struct timeval)},

	// 20
	{"chdir", (syscall_cb)task_chdir, 0,
		SCA_STRING, 0, 0, 0},

	// 21
	{"utimes", (syscall_cb)vfs_utimes, SCF_TASKEND,
		SCA_STRING, 0, 0, 0},

	// 22
	{"fork", (syscall_cb)task_fork, SCF_STATE,
		0, 0, 0, 0},

	// 23
	{"rmdir", (syscall_cb)vfs_rmdir,
		SCF_TASKEND, SCA_STRING, 0, 0, 0},

#ifdef ENABLE_PICOTCP
	// 24
	{"socket", (syscall_cb)net_socket, 0,
		SCA_INT, SCA_INT, SCA_INT, 0},

	// 25
	{"bind", (syscall_cb)net_bind, 0,
		SCA_INT, SCA_POINTER | SCA_SIZE_IN_2, SCA_INT, 0},
#else
	// 24
	{"socket", NULL, 0,
		0, 0, 0, 0},

	// 25
	{"bind", NULL, 0,
		0, 0, 0, 0},
#endif

	// 26
	/* FIXME Size incorrect */
	{"ioctl", (syscall_cb)vfs_ioctl, SCF_TASKEND,
		SCA_INT, SCA_INT, SCA_POINTER | SCA_NULLOK, 0x600},

	// 27
	{"", NULL, 0,
		0, 0, 0, 0},

	// 28
	{"pipe", (syscall_cb)vfs_pipe, 0,
		SCA_POINTER, 0, 0, sizeof(int) * 2},

	// 29
	{"waitpid", (syscall_cb)task_waitpid, 0,
		SCA_INT, SCA_POINTER | SCA_NULLOK, SCA_INT, sizeof(int)},

	// 30
	{"", NULL, 0,
		0, 0, 0, 0},

	// 31
	{"readlink", (syscall_cb)vfs_readlink, SCF_TASKEND,
		SCA_STRING, SCA_POINTER | SCA_SIZE_IN_2, SCA_INT, 0},

	// 32
	/* FIXME Size incorrect */
	{"execve", (syscall_cb)task_execve, 0,
		SCA_STRING, SCA_POINTER, SCA_POINTER, 0x600},

	// 33
	{"sigaction", (syscall_cb)task_sigaction, 0,
		SCA_INT, SCA_POINTER | SCA_NULLOK,
		SCA_POINTER | SCA_NULLOK, sizeof(struct sigaction)},

	// 34
	{"sigprocmask", (syscall_cb)task_sigprocmask, 0,
		SCA_INT, SCA_POINTER | SCA_NULLOK,
		SCA_POINTER | SCA_NULLOK, sizeof(uint32_t)},

	// 35
	{"sigsuspend", NULL, 0,
		0, 0, 0, 0},

	// 36
	{"fcntl", (syscall_cb)vfs_fcntl, SCF_TASKEND,
		SCA_INT, SCA_INT, SCA_INT, 0},

#ifdef ENABLE_PICOTCP
	/* FIXME Pointer sizes missing */
	// 37
	{"listen", (syscall_cb)net_listen, 0,
		SCA_INT, SCA_INT, 0, 0},

	// 38
	{"accept", (syscall_cb)net_accept, 0,
		SCA_INT, SCA_POINTER, SCA_POINTER, 0},

	// 39
	{"select", (syscall_cb)net_select, 0,
		SCA_INT, SCA_POINTER, SCA_POINTER, 0},

	// 40
	{"getpeername", (syscall_cb)net_getpeername, 0,
		SCA_INT, SCA_POINTER, SCA_POINTER, 0},

	// 41
	{"getsockname", (syscall_cb)net_getsockname, 0,
		SCA_INT, SCA_POINTER, SCA_POINTER, 0},
#else
	// 37
	{"socket", NULL, 0,
		0, 0, 0, 0},

	// 38
	{"accept", NULL, 0,
		0, 0, 0, 0},

	// 39
	{"select", NULL, 0,
		0, 0, 0, 0},

	// 40
	{"getpeername", NULL, 0,
		0, 0, 0, 0},

	// 41
	{"getsockname", NULL, 0,
		0, 0, 0, 0},
#endif

	// 42
	{"setid", (syscall_cb)task_setid, 0,
		SCA_INT, SCA_INT, 0, 0},

	// 43
	{"stat", (syscall_cb)vfs_stat, SCF_TASKEND,
		SCA_STRING, SCA_POINTER, 0, sizeof(vfs_stat_t)},

	// 44
	{"dup2", (syscall_cb)vfs_dup2, SCF_TASKEND,
		SCA_INT, SCA_INT, 0, 0},

	// 45
	{"strace", (syscall_cb)task_strace, SCF_STATE,
		0, 0, 0, 0},
};
