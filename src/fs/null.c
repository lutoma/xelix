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

static size_t null_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	if(ctx->fp->meta == 1) {
		bzero(dest, size);
		return size;
	} else {
		return 0;
	}
}

static size_t null_write(struct vfs_callback_ctx* ctx, void* source, size_t size) {
	return size;
}

void vfs_null_init(void) {
	struct vfs_callbacks sfs_cb = {
		.read = null_read,
		.write = null_write,
	};

	struct sysfs_file* null = sysfs_add_dev("null", &sfs_cb);
	struct sysfs_file* zero = sysfs_add_dev("zero", &sfs_cb);
	null->meta = (void*)0;
	zero->meta = (void*)1;
}
