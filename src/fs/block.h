#pragma once

/* Copyright © 2018-2020 Lukas Martini
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

#include <stdbool.h>

struct vfs_block_dev;
typedef uint64_t (*vfs_block_read_cb)(struct vfs_block_dev* dev, uint64_t lba, uint64_t num_blocks, void* buf);
typedef uint64_t (*vfs_block_write_cb)(struct vfs_block_dev* dev, uint64_t lba, uint64_t num_blocks, void* buf);

struct vfs_block_dev {
	struct vfs_block_dev* next;
	char name[50];
	int block_size;

	// Used for partitions
	uint64_t start_offset;

	vfs_block_read_cb read_cb;
	vfs_block_read_cb write_cb;

	// For use by device driver
	void* meta;
};

uint64_t vfs_block_read(struct vfs_block_dev* dev, uint64_t start_block, uint64_t num_blocks, uint8_t* buf);
uint64_t vfs_block_write(struct vfs_block_dev* dev, uint64_t start_block, uint64_t num_blocks, uint8_t* buf);

uint64_t vfs_block_sread(struct vfs_block_dev* dev, uint64_t offset, uint64_t size, uint8_t* buf);
uint64_t vfs_block_swrite(struct vfs_block_dev* dev, uint64_t offset, uint64_t size, uint8_t* buf);

struct vfs_block_dev* vfs_block_get_dev(char* path);
void vfs_block_register_dev(char* name, uint64_t start_offset,
	vfs_block_read_cb read_cb, vfs_block_write_cb write_cb, void* meta);
