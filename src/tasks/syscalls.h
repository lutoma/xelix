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

#define DEFINE_SYSCALL(name) extern uint32_t sys_ ## name (struct syscall syscall);

DEFINE_SYSCALL(exit);
DEFINE_SYSCALL(write);
DEFINE_SYSCALL(access);
DEFINE_SYSCALL(read);
DEFINE_SYSCALL(sbrk);
DEFINE_SYSCALL(unlink);
DEFINE_SYSCALL(chmod);
DEFINE_SYSCALL(link);
DEFINE_SYSCALL(test);
DEFINE_SYSCALL(open);
DEFINE_SYSCALL(stat);
DEFINE_SYSCALL(seek);
DEFINE_SYSCALL(getdents);
DEFINE_SYSCALL(chown);
DEFINE_SYSCALL(kill);
DEFINE_SYSCALL(chdir);
DEFINE_SYSCALL(utimes);
DEFINE_SYSCALL(fork);
DEFINE_SYSCALL(rmdir);
DEFINE_SYSCALL(socket);
DEFINE_SYSCALL(bind);
DEFINE_SYSCALL(socket_send);
DEFINE_SYSCALL(socket_recv);
DEFINE_SYSCALL(wait);
DEFINE_SYSCALL(audio_play);
DEFINE_SYSCALL(close);
DEFINE_SYSCALL(mkdir);
DEFINE_SYSCALL(gettimeofday);
DEFINE_SYSCALL(readlink);
DEFINE_SYSCALL(execve);
DEFINE_SYSCALL(sigaction);

syscall_t syscall_table[] = {
	NULL,
	sys_exit,			// 1
	sys_read,			// 2
	sys_write,			// 3
	sys_access,			// 4
	sys_close,			// 5
	sys_mkdir,			// 6
	sys_sbrk,			// 7
	NULL,				// 8
	sys_test,			// 9
	sys_unlink,			// 10
	sys_chmod,			// 11
	sys_link,			// 12
	sys_open,			// 13
	sys_stat,			// 14
	sys_seek,			// 15
	sys_getdents,		// 16
	sys_chown,			// 17
	sys_kill,			// 18
	sys_gettimeofday,	// 19
	sys_chdir,			// 20
	sys_utimes,			// 21
	sys_fork,			// 22
	sys_rmdir,			// 23
	sys_socket,			// 24
	sys_bind,			// 25
	sys_socket_send,	// 26
	sys_socket_recv,	// 27
	NULL,				// 28
	sys_wait,			// 29
	sys_audio_play,		// 30
	sys_readlink,		// 31
	sys_execve,			// 32
	sys_sigaction,		// 33
};

char* syscall_name_table[] = {
	NULL,
	"exit",			// 1
	"read",			// 2
	"write",		// 3
	"access",		// 4
	"close",		// 5
	"mkdir",		// 6
	"sbrk",			// 7
	"symlink",		// 8
	"test",			// 9
	"unlink",		// 10
	"chmod",		// 11
	"link",			// 12
	"open",			// 13
	"stat",			// 14
	"seek",			// 15
	"getdents",		// 16
	"chown",		// 17
	"kill",			// 18
	"gettimeofday",	// 19
	"chdir",		// 20
	"utimes",		// 21
	"fork",			// 22
	"rmdir",		// 23
	"socket",		// 24
	"bind",			// 25
	"socket_send",	// 26
	"socket_recv",	// 27
	NULL,			// 28
	"wait",			// 29
	"audio_play",	// 30
	"readlink",		// 31
	"execve",		// 32
	"sigaction",	// 33
};
