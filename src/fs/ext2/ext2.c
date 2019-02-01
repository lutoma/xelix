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
#include <time.h>
#include <memory/kmalloc.h>
#include <hw/ide.h>
#include <hw/serial.h>
#include <fs/vfs.h>
#include <fs/ext2.h>
#include <fs/block.h>

int ext2_chmod(const char* path, uint32_t mode) {
	uint32_t inode_num = ext2_resolve_inode(path, NULL);
	if(!inode_num) {
		sc_errno = ENOENT;
		return -1;
	}

	struct inode* inode = kmalloc(superblock->inode_size);
	if(!ext2_inode_read(inode, inode_num)) {
		kfree(inode);
		sc_errno = ENOENT;
		return -1;
	}

	inode->mode = vfs_mode_to_filetype(inode->mode) + mode;
	ext2_inode_write(inode, inode_num);
	kfree(inode);
	return 0;
}


int ext2_unlink(char* path) {
	uint32_t dir_ino = 0;
	uint32_t inode_num = ext2_resolve_inode(path, &dir_ino);
	if(!inode_num || !dir_ino) {
		sc_errno = ENOENT;
		return -1;
	}

	if(inode_num == ROOT_INODE) {
		sc_errno = EPERM;
		return -1;
	}

	struct inode* inode = kmalloc(superblock->inode_size);
	if(!ext2_inode_read(inode, inode_num)) {
		kfree(inode);
		sc_errno = ENOENT;
		return -1;
	}

	ext2_dirent_rm(dir_ino, vfs_basename(path));
	inode->link_count--;

	if(inode->link_count < 1) {
		inode->deletion_time = time_get();

		// TODO Also adjust bitmaps and delete blocks
		struct blockgroup* blockgroup = blockgroup_table + inode_to_blockgroup(inode_num);
		superblock->free_inodes++;
		blockgroup->free_inodes++;
		write_superblock();
		write_blockgroup_table();
	}

	ext2_inode_write(inode, inode_num);
	kfree(inode);
	return 0;
}

int ext2_stat(vfs_file_t* fp, vfs_stat_t* dest) {
	if(!fp || !fp->inode) {
		log(LOG_ERR, "ext2: ext2_stat called without fp or fp missing inode.\n");
		return -1;
	}

	struct inode* inode = kmalloc(superblock->inode_size);
	if(!ext2_inode_read(inode, fp->inode)) {
		kfree(inode);
		return -1;
	}

	dest->st_dev = 1;
	dest->st_ino = fp->inode;
	dest->st_mode = inode->mode;
	dest->st_nlink = inode->link_count;
	dest->st_uid = inode->uid;
	dest->st_gid = inode->gid;
	dest->st_rdev = 0;
	dest->st_size = inode->size;
	dest->st_atime = inode->access_time;
	dest->st_mtime = inode->modification_time;
	dest->st_ctime = inode->creation_time;
	dest->st_blksize = bl_off(1);
	dest->st_blocks = inode->block_count;

	kfree(inode);
	return 0;
}

int ext2_mkdir(const char* path, uint32_t mode) {
	char* name;
	char* base_path = ext2_chop_path(path, &name);

	uint32_t parent_inode = ext2_resolve_inode(base_path, NULL);
	if(!parent_inode) {
		kfree(base_path);
		sc_errno = ENOENT;
		return -1;
	}

	struct inode* inode = kmalloc(superblock->inode_size);
	uint32_t inode_num = ext2_inode_new(inode, FT_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
	ext2_dirent_add(parent_inode, inode_num, name, EXT2_DIRENT_FT_DIR);

	// Create empty dirent block
	struct dirent* buf = (struct dirent*)zmalloc(bl_off(1));
	buf->inode = 0;
	buf->name_len = 0;
	buf->record_len = bl_off(1);

	if(!ext2_inode_write_data(inode, inode_num, 0, sizeof(struct dirent), (uint8_t*)buf)) {
		sc_errno = ENOENT;
		return -1;
	}

	inode->size = bl_off(1);
	ext2_inode_write(inode, inode_num);

	// Add . and .. dirents
	ext2_dirent_add(inode_num, inode_num, ".", EXT2_DIRENT_FT_DIR);
	ext2_dirent_add(inode_num, parent_inode, "..", EXT2_DIRENT_FT_DIR);

	kfree(inode);
	kfree(base_path);
	return 0;
}


void ext2_init() {
	// The superblock always has an offset of 1024, so is in sector 2 & 3
	superblock = (struct superblock*)kmalloc(1024);
	vfs_block_read(1024, sizeof(struct superblock), (uint8_t*)superblock);

	if(superblock->magic != SUPERBLOCK_MAGIC)
	{
		log(LOG_ERR, "ext2: Invalid magic\n");
		return;
	}

	log(LOG_INFO, "ext2: Have ext2 revision %d. %d free / %d blocks.\n",
			superblock->revision, superblock->free_blocks,
			superblock->block_count);


	// Check if the file system is marked as clean
	if(superblock->state != SUPERBLOCK_STATE_CLEAN)
	{
		log(LOG_ERR, "ext2: File system is not marked as clean.\n"
			"Please run fsck.ext2 on it.\n");
		return;
	}

	if(bl_off(1) != 1024) {
		log(LOG_ERR, "ext2: Block sizes != 1024 are not supported right now.\n");
		return;
	}

	// TODO Compare superblocks to each other?

	// RO is irrelevant for now since we're read-only anyways.
	//if(superblock->features_incompat || superblock->features_ro)
	if(superblock->features_incompat)
	{
		log(LOG_WARN, "ext2: This filesystem uses some extensions "
			"which we don't support (incompat: 0x%x, ro: 0x%x)\n",
			superblock->features_incompat, superblock->features_ro);
		//return;
	}

	if(superblock->features_compat)
	{
		log(LOG_INFO, "ext2: This file system supports additional special "
			"features. We'll ignore them (0x%x).\n", superblock->features_compat);
	}

	debug("Loaded ext2 superblock. inode_count=%d, block_count=%d, block_size=%d\n",
		superblock->inode_count, superblock->block_count,
		superblock_to_blocksize(superblock));

	blockgroup_table = kmalloc(superblock_to_blocksize(superblock)
		* blockgroup_table_size);

	if(!vfs_block_read(bl_off(2), bl_off(blockgroup_table_size), (uint8_t*)blockgroup_table)) {
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

	struct vfs_callbacks cb = {
		.open = ext2_open,
		.stat = ext2_stat,
		.read = ext2_read,
		.write = ext2_write,
		.getdents = ext2_getdents,
		.unlink = ext2_unlink,
		.chmod = ext2_chmod,
		.symlink = NULL,
		.mkdir = ext2_mkdir,
	};
	vfs_mount("/", NULL, "/dev/ide1p1", "ext2", &cb);
}

#endif /* ENABLE_EXT2 */
