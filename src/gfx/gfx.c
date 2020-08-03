/* gfx.c: Graphics buffer management/multiplexing
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

#include <gfx/gfx.h>
#include <gfx/fbtext.h>
#include <gfx/mouse.h>
#include <gfx/gfxbus.h>
#include <panic.h>
#include <errno.h>
#include <fs/sysfs.h>
#include <mem/palloc.h>
#include <mem/kmalloc.h>
#include <boot/multiboot.h>

static struct multiboot_tag_framebuffer* fb_desc;
static struct gfx_handle handles[20];
static struct gfx_handle* active_handle = NULL;
static int next_handle = 0;

static void map_handle(struct gfx_handle* handle, bool direct) {
	void* dest;
	if(direct) {
		dest = (void*)(uintptr_t)fb_desc->common.framebuffer_addr;
	} else {
		dest = handle->buf_addr;
	}

	int flags = VM_RW;
	if(handle->ctx) {
		flags |= VM_USER;
	}

	vmem_map(handle->ctx, handle->addr, dest, handle->size, flags);
}

void gfx_handle_enable(unsigned int which) {
	if(which >= 20) {
		return;
	}

	struct gfx_handle* handle = &handles[which];
	if(!handle->used || handle == active_handle) {
		return;
	}

	if(active_handle) {
		memcpy(active_handle->buf_addr, (void*)(uintptr_t)fb_desc->common.framebuffer_addr, active_handle->size);
		map_handle(active_handle, false);
	}

	memcpy((void*)(uintptr_t)fb_desc->common.framebuffer_addr, handle->buf_addr, handle->size);
	map_handle(handle, true);
	active_handle = handle;
}

struct gfx_handle* gfx_handle_init(struct vmem_context* ctx) {
	if(next_handle >= 20) {
		return NULL;
	}

	struct gfx_handle* handle = &handles[next_handle];
	handle->used = true;
	handle->ctx = ctx;
	handle->id = next_handle;
	handle->size = fb_desc->common.framebuffer_width
		* fb_desc->common.framebuffer_height
		* fb_desc->common.framebuffer_bpp;

	handle->buf_addr = palloc(ALIGN(handle->size, PAGE_SIZE) / PAGE_SIZE);
	if(!handle->buf_addr) {
		handle->used = false;
		return NULL;
	}

	if(ctx) {
		// FIXME Properly choose virtual userland pages
		handle->addr = (void*)0xf4000000;
	} else {
		handle->addr = handle->buf_addr;
	}

	handle->bpp = fb_desc->common.framebuffer_bpp;
	handle->width = fb_desc->common.framebuffer_width;
	handle->height = fb_desc->common.framebuffer_height;
	handle->pitch = fb_desc->common.framebuffer_pitch;

	map_handle(handle, false);
	next_handle++;
	return handle;
}

static int sfs_ioctl(struct vfs_callback_ctx* ctx, int request, void* _arg) {
	if(request == 0x2f01) {
		bool copied = false;
		struct gfx_handle* user_handle = task_memmap(ctx->task, _arg, sizeof(struct gfx_handle), &copied);
		if(!user_handle) {
			sc_errno = EINVAL;
			return -1;
		}


		struct gfx_handle* handle = gfx_handle_init(ctx->task->vmem_ctx);
		if(!handle) {
			return -1;
		}

		memcpy(user_handle, handle, sizeof(struct gfx_handle));

		if(copied) {
			task_memcpy(ctx->task, user_handle, _arg, sizeof(struct gfx_handle), true);
			kfree(user_handle);
		}

		return 0;

	} else if(request == 0x2f02) {
		gfx_handle_enable((unsigned int)_arg);
		return 0;
	}

	sc_errno = ENOSYS;
	return -1;
}

void gfx_init() {
	fb_desc = multiboot_get_framebuffer();
	if(!fb_desc) {
		panic("Could not initialize graphics handling - No multiboot framebuffer");
		return;
	}

	log(LOG_DEBUG, "gfx1: %dx%d bpp %d pitch 0x%x at 0x%x\n",
		fb_desc->common.framebuffer_width,
		fb_desc->common.framebuffer_height,
		fb_desc->common.framebuffer_bpp,
		fb_desc->common.framebuffer_pitch,
		(uint32_t)fb_desc->common.framebuffer_addr);

	// Map the framebuffer into the kernel paging context
	size_t vmem_size = fb_desc->common.framebuffer_width
      	* fb_desc->common.framebuffer_height
		* fb_desc->common.framebuffer_bpp;
	vmem_map_flat(NULL, (void*)(uint32_t)fb_desc->common.framebuffer_addr, vmem_size, VM_RW);

	struct vfs_callbacks sfs_cb = {
		.ioctl = sfs_ioctl,
	};
	sysfs_add_dev("gfx1", &sfs_cb);

	gfx_mouse_init();
	gfx_fbtext_init();
	tty_gfxbus_init();
}
