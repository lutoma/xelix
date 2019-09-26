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
#include "inode.h"
#include <log.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <mem/kmalloc.h>
#include <fs/vfs.h>
#include <fs/ext2.h>
#include <fs/block.h>
#include "cb.h"


struct vfs_callbacks cb = {
	.open = ext2_open,
	.stat = ext2_stat,
	.read = ext2_read,
	.write = ext2_write,
	.getdents = ext2_getdents,
	.unlink = ext2_unlink,
	.chmod = ext2_chmod,
	.chown = ext2_chown,
	.symlink = NULL,
	.mkdir = ext2_mkdir,
	.utimes = ext2_utimes,
	.rmdir = ext2_rmdir,
	.link = ext2_link,
	.readlink = ext2_readlink,
	.access = ext2_access,
};

void ext2_init(struct vfs_block_dev* dev) {
	ext2_block_dev = dev;

	// Main superblock always has an offset of 1024
	superblock = (struct superblock*)kmalloc(1024);
	if(vfs_block_read(ext2_block_dev, 2, 2, (uint8_t*)superblock) < 0 ||
		superblock->magic != SUPERBLOCK_MAGIC) {
		log(LOG_ERR, "ext2: Invalid magic\n");
		return;
	}


	log(LOG_INFO, "ext2: Revision %d, block size %d, %d blockgroups\n",
			superblock->revision, bl_off(1), superblock->blockgroup_num);
	log(LOG_INFO, "ext2: Blocks: %d free / %d total\n",
		superblock->free_blocks, superblock->block_count);
	log(LOG_INFO, "ext2: Inodes: %d free / %d total\n",
		superblock->free_inodes, superblock->inode_count);

	if(superblock->state != SUPERBLOCK_STATE_CLEAN) {
		log(LOG_ERR, "ext2: File system is not marked as clean.\n"
			"Please run fsck.ext2 on it.\n");
		return;
	}

	// TODO Compare superblocks to each other?

	// RO is irrelevant for now since we're read-only anyways.
	//if(superblock->features_incompat || superblock->features_ro)
	if(superblock->features_incompat) {
		log(LOG_WARN, "ext2: This filesystem uses some extensions "
			"which we don't support (incompat: 0x%x, ro: 0x%x)\n",
			superblock->features_incompat, superblock->features_ro);
		//return;
	}

	if(superblock->features_compat) {
		log(LOG_INFO, "ext2: This file system supports additional special "
			"features. We'll ignore them (0x%x).\n", superblock->features_compat);
	}

	blockgroup_table = kmalloc(bl_off(blockgroup_table_size));
	if(!vfs_block_sread(ext2_block_dev, bl_off(blockgroup_table_start),
		bl_off(blockgroup_table_size), (uint8_t*)blockgroup_table)) {

		kfree(superblock);
		kfree(blockgroup_table);
		return;
	}

	// Cache root inode
	struct inode* root_inode_buf = kmalloc(superblock->inode_size);
	if(!ext2_inode_read(root_inode_buf, ROOT_INODE)) {
		log(LOG_ERR, "ext2: Could not read root inode.\n");
		kfree(superblock);
		kfree(root_inode_buf);
		kfree(blockgroup_table);
		return;
	}

	root_inode = root_inode_buf;
	superblock->mount_count++;
	superblock->mount_time = time_get();
	write_superblock();

	ext2_callbacks = &cb;
	vfs_mount("/", NULL, dev, "ext2", &cb);
}

#endif /* ENABLE_EXT2 */
