#pragma once

/* Copyright © 2012 Lukas Martini
 *
 * This file is part of Xlibc.
 *
 * Xlibc is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Xlibc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Xlibc. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stddef.h>

#if defined(NDEBUG)
	#undef assert
	#define assert(ignore)((void)0)
#else
	#define assert(args...)((void)(		\
		if(!(args))						\
		{								\
			fprintf(stderr, "Assertion " ## args ## " in " __FILE__ ":" __LINE__ "(" __func__ ") failed"); \
			abort();					\
		}))
#endif
