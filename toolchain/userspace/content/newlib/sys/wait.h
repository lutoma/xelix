/* Copyright © 2016 Lukas Martini
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

#ifndef _SYS_WAIT_H
#define _SYS_WAIT_H

#include <stdint.h>
#include <sys/signal.h>
#include <sys/types.h>

#define WNOHANG 0
#define WUNTRACED 1
#define WEXITSTATUS 2
#define WIFCONTINUED 3
#define WIFEXITED 4
#define WIFSIGNALED 5
#define WIFSTOPPED 6
#define WSTOPSIG 7
#define WTERMSIG 8
#define WEXITED 9
#define WSTOPPED 10
#define WCONTINUED 11
#define WNOHANG 12
#define WNOWAIT 13

// FIXME
#define WEXITSTATUS(args...) (args)

typedef uint32_t idtype_t;

/* These should actually be defined in sys/types.h & sys/signal.h according to
 * POSIX and just redefined here. newlib doesn't define them there though. */
typedef uint32_t id_t;


pid_t wait(int*);
int waitid(idtype_t, id_t, siginfo_t*, int);
pid_t waitpid(pid_t, int*, int);

#endif /* _SYS_WAIT_H */
