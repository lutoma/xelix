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
/*
#ifndef _SYS_SELECT_H
#define _SYS_SELECT_H

#include <sys/time.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>

int pselect(int nfds, fd_set* __restrict__ readfds,
       fd_set* __restrict__ writefds, fd_set* __restrict__ errorfds,
       const struct timespec* __restrict__ timeout,
       const sigset_t* __restrict__ sigmask);

int select(int nfds, fd_set* __restrict__ readfds,
       fd_set* __restrict__ writefds, fd_set* __restrict__ errorfds,
       struct timeval* __restrict__ timeout);

#endif /* _SYS_SELECT_H */
