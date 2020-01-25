#pragma once

/* Copyright © 2020 Lukas Martini
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

struct vfs_callback_ctx;
struct vfs_mountpoint {
	struct vfs_mountpoint* prev;
	struct vfs_mountpoint* next;
	char path[265];
	void* instance;
	char type[50];
	struct vfs_block_dev* dev;
	struct vfs_callbacks callbacks;
};

int vfs_mount_register(struct vfs_block_dev* dev, const char* path, void* instance, const char* type,
	struct vfs_callbacks* callbacks);
struct vfs_mountpoint* vfs_mount_get(const char* path, char** mount_path);
int vfs_mount(struct task* task, const char* source, const char* target, int flags);
int vfs_umount(struct task* task, const char* target, int flags);
void vfs_mount_init(const char* root_path);
