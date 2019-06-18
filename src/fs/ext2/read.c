/* ext2.c: Implementation of the extended file system, version 2
 * Copyright © 2013-2018 Lukas Martini
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
#include <md5.h>
#include <mem/kmalloc.h>
#include <fs/vfs.h>
#include <fs/ext2.h>

size_t ext2_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	if(!ctx->fp || !ctx->fp->inode) {
		log(LOG_ERR, "ext2: ext2_read_file called without fp or fp missing inode.\n");
		sc_errno = EBADF;
		return -1;
	}

	debug("ext2_read_file for %s, off %d, size %d\n", ctx->fp->mount_path, ctx->fp->offset, size);

	struct inode* inode = kmalloc(superblock->inode_size);
	if(!ext2_inode_read(inode, ctx->fp->inode)) {
		kfree(inode);
		sc_errno = EBADF;
		return -1;
	}

	if(ext2_inode_check_perm(PERM_CHECK_READ, inode, ctx->task) < 0) {
		kfree(inode);
		sc_errno = EACCES;
		return -1;
	}

	debug("%s uid=%d, gid=%d, size=%d, ft=%s mode=%s\n", ctx->fp->mount_path, inode->uid,
		inode->gid, inode->size, vfs_filetype_to_verbose(vfs_mode_to_filetype(inode->mode)),
		vfs_get_verbose_permissions(inode->mode));

	if(vfs_mode_to_filetype(inode->mode) != FT_IFREG)
	{
		debug("ext2_read_file: Attempt to read something weird "
			"(0x%x: %s)\n", inode->mode,
			vfs_filetype_to_verbose(vfs_mode_to_filetype(inode->mode)));

		kfree(inode);
		sc_errno = EISDIR;
		return -1;
	}

	if(inode->size < 1 || ctx->fp->offset >= inode->size) {
		kfree(inode);
		return 0;
	}

	if(ctx->fp->offset + size > inode->size) {
		size = inode->size - ctx->fp->offset;
		debug("ext2: Capping read size to 0x%x\n", size);
	}

	uint8_t* read = ext2_inode_read_data(inode, ctx->fp->offset, size, dest);
	kfree(inode);

	if(!read) {
		return 0;
	}
	return size;
}

#endif /* ENABLE_EXT2 */
