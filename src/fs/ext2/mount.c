/* mount.c: Ext2 filesystem mounting
 * Copyright © 2013-2020 Lukas Martini
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
	.build_path_tree = ext2_build_path_tree,
};

int ext2_mount(struct vfs_block_dev* dev, const char* path) {
	struct ext2_fs* fs = zmalloc(sizeof(struct ext2_fs));
	fs->dev = dev;

	// Main superblock always has an offset of 1024
	fs->superblock = (struct superblock*)kmalloc(1024);
	if(vfs_block_read(fs->dev, 2, 2, (uint8_t*)fs->superblock) < 0 ||
		fs->superblock->magic != SUPERBLOCK_MAGIC) {
		log(LOG_ERR, "ext2: Invalid magic\n");

		kfree(fs);
		return -1;
	}

	log(LOG_INFO, "ext2: Mounting /dev/%s - ext2 revision %d, block size %d, %d blockgroups\n",
			dev->name, fs->superblock->revision, bl_off(1), fs->superblock->blockgroup_num);
	log(LOG_INFO, "ext2: Blocks: %d free / %d total\n",
		fs->superblock->free_blocks, fs->superblock->block_count);
	log(LOG_INFO, "ext2: Inodes: %d free / %d total\n",
		fs->superblock->free_inodes, fs->superblock->inode_count);

	if(fs->superblock->state != SUPERBLOCK_STATE_CLEAN) {
		log(LOG_ERR, "ext2: File system on /dev/%s is not marked as clean. "
			"Please run fsck.ext2 on it.\n", dev->name);

		kfree(fs);
		return -1;
	}

	// TODO Compare superblocks to each other?

	fs->blockgroup_table = kmalloc(bl_off(blockgroup_table_size));
	if(!vfs_block_sread(fs->dev, bl_off(blockgroup_table_start),
		bl_off(blockgroup_table_size), (uint8_t*)fs->blockgroup_table)) {

		kfree(fs->superblock);
		kfree(fs->blockgroup_table);
		kfree(fs);
		return -1;
	}

	// Cache root inode
	struct inode* root_inode_buf = kmalloc(fs->superblock->inode_size);
	if(!ext2_inode_read(fs, root_inode_buf, ROOT_INODE)) {
		log(LOG_ERR, "ext2: Could not read root inode.\n");
		kfree(fs->superblock);
		kfree(fs->blockgroup_table);
		kfree(root_inode_buf);
		kfree(fs);
		return -1;
	}

	fs->root_inode = root_inode_buf;
	fs->superblock->mount_count++;
	fs->superblock->mount_time = time_get();
	fs->callbacks = &cb;
	write_superblock();
	vfs_register_fs(dev, path, (void*)fs, "ext2", &cb);
	return 0;
}

#endif /* ENABLE_EXT2 */
