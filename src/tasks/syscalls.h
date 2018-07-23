#pragma once

/* Copyright © 2014-2016 Lukas Martini
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

#include <lib/generic.h>
#include <tasks/syscall.h>

#define DEFINE_SYSCALL(name) extern void sys_ ## name (struct syscall syscall);

DEFINE_SYSCALL(exit);
DEFINE_SYSCALL(write);
DEFINE_SYSCALL(getpid);
DEFINE_SYSCALL(getppid);
DEFINE_SYSCALL(read);
DEFINE_SYSCALL(mmap);
DEFINE_SYSCALL(munmap);
DEFINE_SYSCALL(test);
DEFINE_SYSCALL(set_hostname);
DEFINE_SYSCALL(get_hostname);
DEFINE_SYSCALL(uname);
DEFINE_SYSCALL(open);
DEFINE_SYSCALL(seek);
DEFINE_SYSCALL(getdents);
DEFINE_SYSCALL(kill);
DEFINE_SYSCALL(getexecdata);
DEFINE_SYSCALL(chdir);
DEFINE_SYSCALL(getcwd);
DEFINE_SYSCALL(fork);
DEFINE_SYSCALL(socket);
DEFINE_SYSCALL(bind);
DEFINE_SYSCALL(socket_send);
DEFINE_SYSCALL(socket_recv);
DEFINE_SYSCALL(execnew);
DEFINE_SYSCALL(wait);
DEFINE_SYSCALL(audio_play);

syscall_t syscall_table[] = {
	NULL,
	sys_exit,			// 1
	sys_read,			// 2
	sys_write,			// 3
	sys_getpid,			// 4
	NULL,				// 5
	sys_getppid,		// 6
	sys_mmap,			// 7
	sys_munmap,			// 8
	sys_test,			// 9
	sys_get_hostname,	// 10
	sys_set_hostname,	// 11
	sys_uname,			// 12
	sys_open,			// 13
	NULL,				// 14
	sys_seek,			// 15
	sys_getdents,		// 16
	NULL,				// 17
	sys_kill,			// 18
	sys_getexecdata,	// 19
	sys_chdir,			// 20
	sys_getcwd,			// 21
	sys_fork,			// 22
	NULL,				// 23
	sys_socket,			// 24
	sys_bind,			// 25
	sys_socket_send,	// 26
	sys_socket_recv,	// 27
	sys_execnew,		// 28
	sys_wait,			// 29
	sys_audio_play		// 30
};

char* syscall_name_table[] = {
	NULL,
	"exit",			// 1
	"read",			// 2
	"write",		// 3
	"getpid",		// 4
	NULL,			// 5
	"getppid",		// 6
	"mmap",			// 7
	"munmap",		// 8
	"test",			// 9
	"get_hostname",	// 10
	"set_hostname",	// 11
	"uname",		// 12
	"open",			// 13
	NULL,			// 14
	"seek",			// 15
	"getdents",		// 16
	NULL,			// 17
	"kill",			// 18
	"getexecdata",	// 19
	"chdir",		// 20
	"getcwd",		// 21
	"fork",			// 22
	NULL,			// 23
	"socket",		// 24
	"bind",			// 25
	"socket_send",	// 26
	"socket_recv",	// 27
	"execnew",		// 28
	"wait",			// 29
	"audio_play"	// 30
};
