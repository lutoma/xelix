/* block.c: Block device management
 * Copyright © 2018-2020 Lukas Martini
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

#include <block/block.h>
#include <string.h>
#include <mem/kmalloc.h>
#include <block/i386-ide.h>
#include <block/virtio.h>
#include <block/part.h>
#include <block/null.h>
#include <block/random.h>
#include <fs/sysfs.h>
#include <fs/mount.h>
#include <mem/vm.h>
#include <panic.h>

static int num_devs = 0;
static struct vfs_block_dev* block_devs = NULL;

struct vfs_block_dev* vfs_block_get_dev(const char* path) {
	if(strlen(path) < 6 || strncmp(path, "/dev/", 5)) {
		return NULL;
	}

	struct vfs_block_dev* dev = block_devs;
	while(dev) {
		if(!strcmp(dev->name, path + 5)) {
			return dev;
		}
		dev = dev->next;
	}
	return NULL;
}

uint64_t vfs_block_read(struct vfs_block_dev* dev, uint64_t start_block, uint64_t num_blocks, uint8_t* buf) {
	return dev->read_cb(dev, start_block + dev->start_offset, num_blocks, buf);
}

uint64_t vfs_block_write(struct vfs_block_dev* dev, uint64_t start_block, uint64_t num_blocks, uint8_t* buf) {
	return dev->write_cb(dev, start_block + dev->start_offset, num_blocks, buf);
}

uint64_t vfs_block_sread(struct vfs_block_dev* dev, uint64_t position, uint64_t size, uint8_t* buf) {
	int start_block = position / dev->block_size;
	uint64_t offset = (position % dev->block_size);
	int num_blocks = (size + offset + dev->block_size - 1) / dev->block_size;

	if(!offset && !(size % dev->block_size)) {
		return vfs_block_read(dev, start_block, num_blocks, buf) * dev->block_size;
	}

	vm_alloc_t alloc;
	uint64_t buffer_size = num_blocks * dev->block_size;
	assert(buffer_size >= size);

	uint8_t* int_buf = vm_alloc(VM_KERNEL, &alloc, RDIV(buffer_size, PAGE_SIZE), NULL, 0);

	if(vfs_block_read(dev, start_block, num_blocks, int_buf) < num_blocks) {
		kfree(int_buf);
		return -1;
	}

	memcpy(buf, int_buf + offset, size);
	vm_free(&alloc);
	return size;
}

uint64_t vfs_block_swrite(struct vfs_block_dev* dev, uint64_t position, uint64_t size, uint8_t* buf) {
	int start_block = position / dev->block_size;
	uint64_t offset = (position % dev->block_size);
	int num_blocks = (size + offset + dev->block_size - 1) / dev->block_size;

	if(!offset && !(size % dev->block_size)) {
		return vfs_block_write(dev, start_block, num_blocks, buf) * dev->block_size;
	}

	// FIXME Should only read the required parts at the beginning and end
	uint8_t* int_buf = kmalloc(num_blocks * dev->block_size);
	if(vfs_block_read(dev, start_block, num_blocks, int_buf) < num_blocks) {
		kfree(int_buf);
		return -1;
	}

	memcpy(int_buf + offset, buf, size);

	if(vfs_block_write(dev, start_block, num_blocks, int_buf) < num_blocks) {
		kfree(int_buf);
		return -1;
	}

	kfree(int_buf);
	return size;
}

static size_t sfs_block_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	struct vfs_block_dev* dev = (struct vfs_block_dev*)ctx->fp->meta;
	if(!dev) {
		return -1;
	}

	if(!vfs_block_sread(dev, ctx->fp->offset, size, dest)) {
		return -1;
	}
	return size;
}

static size_t sfs_block_write(struct vfs_callback_ctx* ctx, void* src, size_t size) {
	struct vfs_block_dev* dev = (struct vfs_block_dev*)ctx->fp->meta;
	if(!dev) {
		return -1;
	}

	if(!vfs_block_swrite(dev, ctx->fp->offset, size, src)) {
		return -1;
	}
	return size;
}

static int sfs_block_stat(struct vfs_callback_ctx* ctx, vfs_stat_t* dest) {
	struct vfs_block_dev* dev = vfs_block_get_dev(ctx->orig_path);
	if(!dev) {
		return -1;
	}

	dest->st_dev = 2;
	dest->st_ino = 1;
	dest->st_mode = FT_IFBLK;
	dest->st_mode |= S_IRUSR | S_IRGRP | S_IROTH;
	dest->st_mode |= S_IWUSR | S_IWGRP | S_IWOTH;
	dest->st_nlink = 1;
	dest->st_blocks = 0;
	dest->st_blksize = dev->block_size;
	dest->st_uid = 0;
	dest->st_gid = 0;
	dest->st_rdev = 0;
	dest->st_size = 0;
	uint32_t t = time_get();
	dest->st_atime = t;
	dest->st_mtime = t;
	dest->st_ctime = t;
	return 0;
}

void vfs_block_register_dev(char* name, uint64_t start_offset,
	vfs_block_read_cb read_cb, vfs_block_write_cb write_cb, void* meta) {

	struct vfs_block_dev* dev = zmalloc(sizeof(struct vfs_block_dev));
	strcpy(dev->name, name);
	dev->start_offset = start_offset;
	dev->block_size = 512;
	dev->read_cb = read_cb;
	dev->write_cb = write_cb;
	dev->meta = meta;
	dev->number = ++num_devs;
	dev->next = block_devs;
	block_devs = dev;

	// Add /dev file
	struct vfs_callbacks sfs_block_cb = {
		.read = sfs_block_read,
		.write = sfs_block_write,
		.stat = sfs_block_stat,
	};
	struct sysfs_file* sfp = sysfs_add_dev(name, &sfs_block_cb);
	sfp->meta = (void*)dev;

	// Probe for partitions unless this is a partition
	if(!dev->start_offset) {
		vfs_part_probe(dev);
	}
}

void block_init(void) {
	ide_init();

	#ifdef CONFIG_ENABLE_VIRTIO_BLOCK
	virtio_block_init();
	#endif

	block_null_init();
	block_random_init();
}
