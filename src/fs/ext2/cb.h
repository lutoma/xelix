#pragma once

/* Copyright © 2013-2019 Lukas Martini
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

#include <fs/vfs.h>

int ext2_chmod(struct vfs_callback_ctx* ctx, uint32_t mode);
int ext2_chown(struct vfs_callback_ctx* ctx, uint16_t uid, uint16_t gid);
int ext2_stat(struct vfs_callback_ctx* ctx, vfs_stat_t* dest);
int ext2_mkdir(struct vfs_callback_ctx* ctx, uint32_t mode);
int ext2_utimes(struct vfs_callback_ctx* ctx, struct timeval times[2]);
int ext2_unlink(struct vfs_callback_ctx* ctx);
int ext2_rmdir(struct vfs_callback_ctx* ctx);
int ext2_link(struct vfs_callback_ctx* ctx, const char* new_path);
int ext2_access(struct vfs_callback_ctx* ctx, uint32_t amode);
int ext2_readlink(struct vfs_callback_ctx* ctx, char* buf, size_t size);
size_t ext2_read(struct vfs_callback_ctx* ctx, void* dest, size_t size);
size_t ext2_write(struct vfs_callback_ctx* ctx, void* source, size_t size);
size_t ext2_getdents(struct vfs_callback_ctx* ctx, void* dest, size_t size);
int ext2_build_path_tree(struct vfs_callback_ctx* ctx);
