/* crt0.c: crt0 for userland tasks
 * Copyright © 2018-2021 Lukas Martini
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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/xelix.h>

int h_errno;
extern int main(int argc, char** argv);
extern void __libc_init_array();
extern void __libc_fini_array();

// These are defined as extern in sys/xelix.h
struct _xelix_execdata* _xelix_execdata;
char* _progname;

void __attribute__((fastcall, noreturn)) _start(void) {
	__libc_init_array();

	_xelix_execdata = (struct _xelix_execdata*)0x5000;
	environ = _xelix_execdata->env;

	atexit(__libc_fini_array);
	int ret = main(_xelix_execdata->argc, _xelix_execdata->argv);
	exit(ret);
	__builtin_unreachable();
}
