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
#include <mem/mem.h>
#include <mem/kmalloc.h>
#include <boot/multiboot.h>

static struct multiboot_tag_framebuffer* fb_desc;

static struct gfx_handle handles[20];
static struct gfx_handle* active_handle = NULL;
static void* framebuffer_addr;
static int next_handle = 0;

struct gfx_ul_blit_cmd {
	unsigned int handle_id;
	size_t x;
	size_t y;
	size_t width;
	size_t height;
};

struct gfx_handle* gfx_get_handle(unsigned int id) {
	if(id >= 20) {
		return NULL;
	}

	struct gfx_handle* handle = &handles[id];
	if(!handle->used) {
		return NULL;
	}

	return handle;
}

void gfx_blit_all(struct gfx_handle* handle) {
	if(handle == active_handle) {
		memcpy((void*)(uintptr_t)framebuffer_addr, handle->buf_addr, handle->size);
	}
}

void gfx_blit(struct gfx_handle* handle, size_t x, size_t y, size_t width, size_t height) {
	if(handle != active_handle) {
		return;
	}

	// Naïve line-based copy
	for(size_t cy = y; cy < y + height && cy < handle->height; cy++) {
		uintptr_t offset = cy * handle->pitch + (x * handle->bpp/8);
		memcpy((void*)((uintptr_t)framebuffer_addr + offset), handle->buf_addr + offset, width * handle->bpp/8);
	}
}

void gfx_handle_enable(struct gfx_handle* handle) {
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
	handle->size = fb_desc->common.framebuffer_height * fb_desc->common.framebuffer_pitch;

	vmem_t vmem;
	if(valloc(VA_KERNEL, &vmem, ALIGN(handle->size, PAGE_SIZE) / PAGE_SIZE, NULL, VM_RW) != 0) {
		handle->used = false;
		return NULL;
	}

	handle->buf_addr = vmem.addr;

	if(ctx) {
		// FIXME Properly choose virtual userland pages
		handle->addr = (void*)0xf4000000;
		vmem_map(handle->ctx, handle->addr, vmem.phys, handle->size + PAGE_SIZE, VM_RW | VM_USER);
	} else {
		handle->addr = handle->buf_addr;
	}

	handle->bpp = fb_desc->common.framebuffer_bpp;
	handle->width = fb_desc->common.framebuffer_width;
	handle->height = fb_desc->common.framebuffer_height;
	handle->pitch = fb_desc->common.framebuffer_pitch;

	next_handle++;
	return handle;
}

#define get_handle_or_einval(x) \
	struct gfx_handle* handle = gfx_get_handle(x); \
	if(!handle) { \
		sc_errno = EINVAL; \
			return -1; \
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
	}

	if(request == 0x2f02) {
		get_handle_or_einval((unsigned int)_arg);
		gfx_handle_enable(handle);
		return 0;
	}

	if(request == 0x2f03) {
		get_handle_or_einval((unsigned int)_arg);
		gfx_blit_all(handle);
		return 0;
	}

	if(request == 0x2f04) {
		bool copied = false;
		struct gfx_ul_blit_cmd* cmd = task_memmap(ctx->task, _arg, sizeof(struct gfx_ul_blit_cmd), &copied);
		if(!cmd) {
			sc_errno = EINVAL;
			return -1;
		}

		get_handle_or_einval(cmd->handle_id);
		gfx_blit(handle, cmd->x, cmd->y, cmd->width, cmd->height);

		if(copied) {
			kfree(cmd);
		}
		return 0;
	}

	sc_errno = EINVAL;
	return -1;
}

void gfx_init() {
	fb_desc = multiboot_get_framebuffer();
	if(!fb_desc) {
		panic("Could not initialize graphics handling - No multiboot framebuffer");
		return;
	}

	// Map the framebuffer into the kernel paging context

	// FIXME do palloc_at to block framebuffer from phys page allocator
	size_t vmem_size = fb_desc->common.framebuffer_height * fb_desc->common.framebuffer_pitch;

	// FIXME use proper APIs
	mem_page_alloc_at(&mem_phys_alloc_ctx, (void*)(uintptr_t)fb_desc->common.framebuffer_addr, ALIGN(vmem_size, PAGE_SIZE) / PAGE_SIZE);

	vmem_t framebuffer_mem;
	valloc(VA_KERNEL, &framebuffer_mem, ALIGN(vmem_size, PAGE_SIZE) / PAGE_SIZE, (void*)(uintptr_t)fb_desc->common.framebuffer_addr, VM_RW);
	framebuffer_addr = framebuffer_mem.addr;

	log(LOG_DEBUG, "gfx1: %dx%d bpp %d pitch 0x%x at 0x%x\n",
		fb_desc->common.framebuffer_width,
		fb_desc->common.framebuffer_height,
		fb_desc->common.framebuffer_bpp,
		fb_desc->common.framebuffer_pitch,
		(uint32_t)framebuffer_addr);

	struct vfs_callbacks sfs_cb = {
		.ioctl = sfs_ioctl,
	};
	sysfs_add_dev("gfx1", &sfs_cb);

	gfx_mouse_init();
	gfx_fbtext_init();
	tty_gfxbus_init();
}
