/* null.c: /dev/null and /dev/zero
 * Copyright © 2018 Lukas Martini
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
 * along with Xelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <fs/sysfs.h>
#include <string.h>

static size_t null_read(void* dest, size_t size, void* meta) {
	if(meta == (void*)1) {
		bzero(dest, size);
		return size;
	} else {
		return 0;
	}
}

static size_t null_write(void* source, size_t size, void* mea) {
	return size;
}

void vfs_null_init(void) {
	sysfs_add_dev("null", null_read, null_write, (void*)0);
	sysfs_add_dev("zero", null_read, null_write, (void*)1);
}
