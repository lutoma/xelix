/* Copyright © 2019 Lukas Martini
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

#include <pwd.h>
#include <stdbool.h>

char* shortname(char* in);
char* readable_fs(uint64_t size);
char* time2str(time_t rtime, char* fmt);
struct passwd* do_auth(char* user);
void run_shell(struct passwd* pwd, bool print_motd);

static inline void *memset32(uint32_t *s, uint32_t v, size_t n) {
	long d0, d1;
	asm volatile("rep stosl"
		: "=&c" (d0), "=&D" (d1)
		: "a" (v), "1" (s), "0" (n)
		: "memory");
	return s;
}
