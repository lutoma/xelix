/* mem.c: Memory management
 * Copyright © 2020 Lukas Martini
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


#include <mem/palloc.h>
#include <mem/kmalloc.h>
#include <mem/vmem.h>
#include <mem/paging.h>
#include <fs/sysfs.h>

static size_t sfs_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	if(ctx->fp->offset) {
		return 0;
	}

	uint32_t kmalloc_total, kmalloc_used;
	uint32_t palloc_total, palloc_used;
	kmalloc_get_stats(&kmalloc_total, &kmalloc_used);
	palloc_get_stats(&palloc_total, &palloc_used);

	size_t rsize = 0;
	sysfs_printf("mem_total: %u\n", palloc_total);
	sysfs_printf("mem_used: %u\n", palloc_used - kmalloc_total + kmalloc_used);
	sysfs_printf("mem_shared: %u\n", 0);
	sysfs_printf("mem_cache: %u\n", 0);
	sysfs_printf("palloc_total: %u\n", palloc_total);
	sysfs_printf("palloc_used: %u\n", palloc_used);
	sysfs_printf("kmalloc_total: %u\n", kmalloc_total);
	sysfs_printf("kmalloc_used: %u\n", kmalloc_used);
	return rsize;
}

void mem_init() {
	palloc_init();
	kmalloc_init();
	vmem_init();

	struct vfs_callbacks sfs_cb = {
		.read = sfs_read,
	};
	sysfs_add_file("mem_info", &sfs_cb);
}
