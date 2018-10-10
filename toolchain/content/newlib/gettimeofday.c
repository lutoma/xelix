/* Copyright © 2018 Lukas Martini
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

#include <stdio.h>
#include <sys/time.h>
#include <errno.h>

FILE* _time_fp = NULL;
int _gettimeofday(struct timeval* p, void* tz) {
	if(!_time_fp) {
		_time_fp = fopen("/sys/time", "r");
	}

	if(!_time_fp) {
		errno = EINVAL;
		return -1;
	}

	uint32_t tsec;
	rewind(_time_fp);
	if(fscanf(_time_fp, "%d", &tsec) != 1) {
		fclose(_time_fp);
		_time_fp = NULL;
		errno = EINVAL;
		return -1;
	}

	p->tv_sec = tsec;
	p->tv_usec = 0;
	return 0;
}

static void __attribute__((destructor)) _close_time_fp(void) {
	if(_time_fp) {
		fclose(_time_fp);
	}
}
