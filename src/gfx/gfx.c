/* gfx.c: Graphics buffer management/multiplexing
 * Copyright © 2020-2023 Lukas Martini
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

void gfx_handle_enable(struct gfx_handle* handle) {
	/*
	if(active_handle == handle) {
		return;
	}


	if(active_handle) {
		serial_printf("handling active handle\n");
		vm_free(&active_handle->vmem);
		vm_alloc_at(active_handle->vm_ctx, &active_handle->vmem, RDIV(handle->ul_desc.size, PAGE_SIZE), active_handle->ul_desc.addr, NULL, VM_RW | VM_USER);
	}

	int_disable();
	vm_free(&handle->vmem);
	vm_alloc_at(handle->vm_ctx, &handle->vmem, RDIV(handle->ul_desc.size, PAGE_SIZE), handle->ul_desc.addr, (void*)(uintptr_t)framebuffer_addr, VM_RW | VM_USER);

	//memcpy(active_handle->buf_addr, (void*)(uintptr_t)framebuffer_addr, handle->size);
	//memcpy((void*)(uintptr_t)framebuffer_addr, handle->buf_addr, handle->size);
	serial_printf("activation done\n");
	*/
	active_handle = handle;
}

struct gfx_handle* gfx_handle_init(struct vm_ctx* ctx) {
	if(next_handle >= 20) {
		return NULL;
	}

	struct gfx_handle* handle = &handles[next_handle];
	handle->used = true;
	handle->id = next_handle;
	handle->vm_ctx = ctx;
	handle->ul_desc.size = fb_desc->common.framebuffer_height * fb_desc->common.framebuffer_pitch;

/*
	if(!vm_alloc(ctx, &handle->vmem, RDIV(handle->ul_desc.size, PAGE_SIZE), NULL, VM_RW)) {
		handle->used = false;
		return NULL;
	}
*/
	if(!vm_alloc(ctx, &handle->vmem, RDIV(handle->ul_desc.size, PAGE_SIZE), (void*)(uintptr_t)fb_desc->common.framebuffer_addr, VM_RW | VM_USER)) {
		handle->used = false;
		return NULL;
	}

	handle->ul_desc.addr = handle->vmem.addr;
	handle->ul_desc.bpp = fb_desc->common.framebuffer_bpp;
	handle->ul_desc.width = fb_desc->common.framebuffer_width;
	handle->ul_desc.height = fb_desc->common.framebuffer_height;
	handle->ul_desc.pitch = fb_desc->common.framebuffer_pitch;

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
		vm_alloc_t alloc;
		struct gfx_ul_desc* user_desc = vm_map(VM_KERNEL, &alloc, &ctx->task->vmem, _arg,
			sizeof(struct gfx_ul_desc), VM_MAP_USER_ONLY | VM_RW);

		if(!user_desc) {
			task_signal(ctx->task, NULL, SIGSEGV, NULL);
			sc_errno = EFAULT;
			return -1;
		}

		struct gfx_handle* handle = gfx_handle_init(&ctx->task->vmem);
		if(!handle) {
			vm_free(&alloc);
			return -1;
		}

		memcpy(user_desc, &handle->ul_desc, sizeof(struct gfx_ul_desc));
		vm_free(&alloc);
		return 0;
	}

	if(request == 0x2f02) {
		get_handle_or_einval((unsigned int)_arg);
		gfx_handle_enable(handle);
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
	size_t vmem_size = fb_desc->common.framebuffer_height * fb_desc->common.framebuffer_pitch;

	// FIXME use proper APIs
	mem_page_alloc_at(&mem_phys_alloc_ctx, (void*)(uintptr_t)fb_desc->common.framebuffer_addr, ALIGN(vmem_size, PAGE_SIZE) / PAGE_SIZE);

	vm_alloc_t framebuffer_mem;
	if(!vm_alloc(VM_KERNEL, &framebuffer_mem, ALIGN(vmem_size, PAGE_SIZE) / PAGE_SIZE, (void*)(uintptr_t)fb_desc->common.framebuffer_addr, VM_RW)) {
		panic("gfx: Could not vm_alloc framebuffer");
	}

	framebuffer_addr = framebuffer_mem.addr;

	log(LOG_DEBUG, "gfx1: %dx%d bpp %d pitch %#x at %p\n",
		fb_desc->common.framebuffer_width,
		fb_desc->common.framebuffer_height,
		fb_desc->common.framebuffer_bpp,
		fb_desc->common.framebuffer_pitch,
		framebuffer_mem.phys);

	struct vfs_callbacks sfs_cb = {
		.ioctl = sfs_ioctl,
	};
	sysfs_add_dev("gfx1", &sfs_cb);

	gfx_mouse_init();
	gfx_fbtext_init();
	tty_gfxbus_init();
}
