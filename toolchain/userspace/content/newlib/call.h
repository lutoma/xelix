/* Copyright © 2013-2016 Lukas Martini
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

#ifndef LIB_CALL_H
#define LIB_CALL_H

extern unsigned int call_open(char *path);
extern unsigned int call_read(unsigned int fd, void *buffer, unsigned int count);
extern unsigned int call_write(unsigned int fd, void *buffer, unsigned int count);
extern unsigned int call_exit(int return_code);
extern void* call_brk(int incr);
extern int call_getpid();
extern int call_seek(int file, int ptr, int dir);
extern int call_kill(int pid, int sig);
extern void* call_mmap(int incr);
extern int call_chdir(const char* path);
extern char* call_getcwd(char *buf, size_t size);
extern int call_fork(void);
extern int call_wait(int *status);
extern pid_t call_execnew(const char* path, char* __argv[], char* __env[]);
#endif

