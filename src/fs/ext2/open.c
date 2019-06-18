/* ext2.c: Implementation of the extended file system, version 2
 * Copyright © 2013-2019 Lukas Martini
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

#ifdef ENABLE_EXT2

#include "ext2_internal.h"
#include <log.h>
#include <string.h>
#include <errno.h>
#include <mem/kmalloc.h>
#include <fs/vfs.h>
#include <fs/ext2.h>

static vfs_file_t* handle_symlink(struct vfs_callback_ctx* ctx, struct inode* inode, uint32_t flags) {
	/* For symlinks with up to 60 chars length, the path is stored in the
	 * inode in the area where normally the block pointers would be.
	 * Otherwise in the file itself.
	 */
	if(inode->size > 60) {
		log(LOG_WARN, "ext2: Symlinks with length >60 are not supported right now.\n");
		kfree(inode);
		return 0;
	}

	char* sym_path = (char*)inode->blocks;
	if(sym_path[0] != '/') {
		char* base_path = ext2_chop_path(ctx->path, NULL);
		ctx->path = vfs_normalize_path(sym_path, base_path);
		kfree(base_path);
	} else {
		ctx->path = strdup(sym_path);
	}

	// FIXME Should be vfs_open to make symlinks across mount points possible
	vfs_file_t* r = ext2_open(ctx, flags);
	return r;
}

// The public open interface to the virtual file system
vfs_file_t* ext2_open(struct vfs_callback_ctx* ctx, uint32_t flags) {
	if(!ctx || !ctx->path || !strcmp(ctx->path, "")) {
		log(LOG_ERR, "ext2: ext2_read_file called with empty path.\n");
		return NULL;
	}

	uint32_t dir_inode = 0;
	struct dirent* dirent = ext2_dirent_find(ctx->path, &dir_inode, ctx->task);
	if(!dirent && (sc_errno != ENOENT || !(flags & O_CREAT))) {
		return NULL;
	}

	uint32_t inode_num = dirent ? dirent->inode : 0;
	struct inode* inode = kmalloc(superblock->inode_size);

	if(!inode_num) {
		debug("ext2_open: Could not find inode, creating one.\n");
		inode_num = ext2_inode_new(inode, FT_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		ext2_dirent_add(dir_inode, inode_num, vfs_basename(ctx->path), EXT2_DIRENT_FT_REG_FILE);

		if(ctx->task) {
			inode->uid = ctx->task->euid;
			inode->gid = ctx->task->egid;
		}
	} else {
		kfree(dirent);

		if((flags & O_CREAT) && (flags & O_EXCL)) {
			kfree(inode);
			sc_errno = EEXIST;
			return NULL;
		}

		if(!ext2_inode_read(inode, inode_num)) {
			kfree(inode);
			sc_errno = ENOENT;
			return NULL;
		}

		if((flags & O_WRONLY) || (flags & O_RDWR)) {
			if(ext2_inode_check_perm(PERM_CHECK_WRITE, inode, ctx->task) < 0) {
				kfree(inode);
				sc_errno = EACCES;
				return NULL;
			}
		}
	}

	uint16_t ft = vfs_mode_to_filetype(inode->mode);
	if(ft == FT_IFDIR && (flags & O_WRONLY || flags & O_RDWR)) {
		kfree(inode);
		sc_errno = EISDIR;
		return NULL;
	}

	if(ft == FT_IFLNK) {
		vfs_file_t* r = handle_symlink(ctx, inode, flags);
		kfree(inode);
		return r;
	}
	kfree(inode);

	vfs_file_t* fp = vfs_alloc_fileno(ctx->task, 3);
	fp->type = ft;
	fp->inode = inode_num;
	memcpy(&fp->callbacks, ext2_callbacks, sizeof(struct vfs_callbacks));
	return fp;
}

#endif /* ENABLE_EXT2 */
