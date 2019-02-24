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
#include <mem/kmalloc.h>
#include <hw/ide.h>
#include <hw/serial.h>
#include <fs/vfs.h>
#include <fs/ext2.h>
#include <fs/block.h>

int ext2_chmod(const char* path, uint32_t mode) {
	struct dirent* dirent = ext2_dirent_find(path, NULL);
	if(!dirent) {
		sc_errno = ENOENT;
		return -1;
	}

	struct inode* inode = kmalloc(superblock->inode_size);
	if(!ext2_inode_read(inode, dirent->inode)) {
		kfree(dirent);
		kfree(inode);
		sc_errno = ENOENT;
		return -1;
	}

	inode->mode = vfs_mode_to_filetype(inode->mode) & mode;
	ext2_inode_write(inode, dirent->inode);
	kfree(dirent);
	kfree(inode);
	return 0;
}

int ext2_chown(const char* path, uint16_t uid, uint16_t gid) {
	struct dirent* dirent = ext2_dirent_find(path, NULL);
	if(!dirent) {
		sc_errno = ENOENT;
		return -1;
	}

	struct inode* inode = kmalloc(superblock->inode_size);
	if(!ext2_inode_read(inode, dirent->inode)) {
		kfree(dirent);
		kfree(inode);
		sc_errno = ENOENT;
		return -1;
	}

	if(uid != -1) {
		inode->uid = uid;
	}
	if(gid != -1) {
		inode->gid = gid;
	}
	ext2_inode_write(inode, dirent->inode);
	kfree(dirent);
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
	dest->st_atime = inode->atime;
	dest->st_mtime = inode->mtime;
	dest->st_ctime = inode->ctime;
	dest->st_blksize = bl_off(1);
	dest->st_blocks = inode->block_count;

	kfree(inode);
	return 0;
}

int ext2_mkdir(const char* path, uint32_t mode) {
	char* name;
	char* base_path = ext2_chop_path(path, &name);

	struct dirent* parent_dirent = ext2_dirent_find(base_path, NULL);
	if(!parent_dirent) {
		kfree(base_path);
		sc_errno = ENOENT;
		return -1;
	}

	struct inode* inode = kmalloc(superblock->inode_size);
	uint32_t inode_num = ext2_inode_new(inode, FT_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

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
	ext2_dirent_add(parent_dirent->inode, inode_num, name, EXT2_DIRENT_FT_DIR);

	// Add . and .. dirents
	ext2_dirent_add(inode_num, inode_num, ".", EXT2_DIRENT_FT_DIR);
	ext2_dirent_add(inode_num, parent_dirent->inode, "..", EXT2_DIRENT_FT_DIR);

	uint32_t blockgroup_num = inode_to_blockgroup(inode_num);
	blockgroup_table[blockgroup_num].used_directories++;
	write_blockgroup_table();

	kfree(parent_dirent);
	kfree(inode);
	kfree(base_path);
	return 0;
}

int ext2_utimes(const char* path, struct timeval times[2]) {
	struct dirent* dirent = ext2_dirent_find(path, NULL);
	if(!dirent) {
		sc_errno = ENOENT;
		return -1;
	}

	struct inode* inode = kmalloc(bl_off(1));
	if(!ext2_inode_read(inode, dirent->inode)) {
		sc_errno = ENOENT;
		return -1;
	}

	if(times) {
		inode->atime = times[0].tv_sec;
		inode->mtime = times[1].tv_sec;
		inode->ctime = times[1].tv_sec;
	} else {
		uint32_t t = time_get();
		inode->atime = t;
		inode->mtime = t;
		inode->ctime = t;
	}

	ext2_inode_write(inode, dirent->inode);
	return 0;
}

static int do_unlink(char* path, bool is_dir) {
	uint32_t dir_ino = 0;
	struct dirent* dirent = ext2_dirent_find(path, &dir_ino);
	if(!dirent || !dir_ino) {
		sc_errno = ENOENT;
		return -1;
	}

	if(dirent->inode == ROOT_INODE) {
		sc_errno = EACCES;
		return -1;
	}

	struct inode* inode = kmalloc(superblock->inode_size);
	if(!ext2_inode_read(inode, dirent->inode)) {
		kfree(inode);
		sc_errno = ENOENT;
		return -1;
	}

	/* link_count on inodes is a uint16_t, which could underflow
	 * with our substractions below if it is set incorrectly, and
	 * then wouldn't match the `if(inode->link_count < 1) {` below.
	 * Transfer to signed int32 to avoid.
	 */
	int32_t link_count = (int32_t)inode->link_count;

	if(is_dir) {
		if(vfs_mode_to_filetype(inode->mode) != FT_IFDIR) {
			sc_errno = ENOTDIR;
			return -1;
		}

		// FIXME Should probably check more throroughly it only contains ./..
		// and has no hard links
		if(link_count > 2) {
			sc_errno = ENOTEMPTY;
			return -1;
		}

		// . and .. inside directory
		link_count -= 2;
	} else {
		if(vfs_mode_to_filetype(inode->mode) == FT_IFDIR) {
			sc_errno = EISDIR;
			return -1;
		}
	}

	ext2_dirent_rm(dir_ino, vfs_basename(path));
	link_count--;
	inode->link_count--;

	if(link_count < 1) {
		debug("do_unlink: inode %d link count < 1, purging.\n", dirent->inode);
		inode->dtime = time_get();
		inode->link_count = 0;

		// Free data blocks
		struct blockgroup* blockgroup = blockgroup_table + inode_to_blockgroup(dirent->inode);
		struct ext2_blocknum_resolver_cache* res_cache = zmalloc(sizeof(struct ext2_blocknum_resolver_cache));

		for(int i = 0; true; i++) {
			uint32_t num = ext2_resolve_blocknum(inode, i, res_cache);
			if(!num) {
				break;
			}

			ext2_bitmap_free(blockgroup->block_bitmap, num % superblock->blocks_per_group);
		}

		ext2_free_blocknum_resolver_cache(res_cache);
		ext2_bitmap_free(blockgroup->inode_bitmap, dirent->inode % superblock->inodes_per_group);
		superblock->free_inodes++;
		blockgroup->free_inodes++;

		// inode->block_count counts IDE blocks, free_blocks counts ext2 blocks.
		superblock->free_blocks += inode->block_count / 2;
		blockgroup->free_blocks += inode->block_count / 2;

		if(is_dir) {
			blockgroup->used_directories--;

			// Decrease parent directory link count (removed .. entry)
			struct inode* dir_inode = kmalloc(superblock->inode_size);
			if(!ext2_inode_read(dir_inode, dir_ino)) {
				kfree(dir_inode);
				kfree(inode);
				sc_errno = ENOENT;
				return -1;
			}

			dir_inode->link_count--;
			ext2_inode_write(dir_inode, dir_ino);
		}

		write_superblock();
		write_blockgroup_table();
	}

	ext2_inode_write(inode, dirent->inode);
	kfree(inode);
	kfree(dirent);
	return 0;
}

int ext2_unlink(char* path) {
	return do_unlink(path, false);
}

int ext2_rmdir(char* path) {
	return do_unlink(path, true);
}

int ext2_link(const char* path, const char* new_path) {
	char* new_name;
	char* new_dir_path = ext2_chop_path(new_path, &new_name);
	struct dirent* dirent = ext2_dirent_find(path, NULL);
	struct dirent* dir_dirent = ext2_dirent_find(new_dir_path, NULL);

	if(!dirent || !dir_dirent) {
		kfree(new_dir_path);
		kfree(dirent);
		sc_errno = ENOENT;
		return -1;
	}

	// Directory hard links are not allowed in ext2
	if(dirent->type == EXT2_DIRENT_FT_DIR) {
		sc_errno = EACCES;
		return -1;
	}

	ext2_dirent_add(dir_dirent->inode, dirent->inode, new_name, dirent->type);
	kfree(new_dir_path);
	kfree(dirent);
	return 0;
}

int ext2_readlink(const char* path, char* buf, size_t size) {
	struct dirent* dirent = ext2_dirent_find(path, NULL);
	if(!dirent) {
		sc_errno = ENOENT;
		return -1;
	}

	struct inode* inode = kmalloc(bl_off(1));
	if(!ext2_inode_read(inode, dirent->inode)) {
		kfree(inode);
		kfree(dirent);
		sc_errno = ENOENT;
		return -1;
	}

	if(vfs_mode_to_filetype(inode->mode) != FT_IFLNK) {
		kfree(inode);
		kfree(dirent);
		sc_errno = EINVAL;
		return -1;
	}

	strncpy(buf, (char*)inode->blocks, size);
	kfree(inode);
	kfree(dirent);
	return strlen(buf);
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
		.chown = ext2_chown,
		.symlink = NULL,
		.mkdir = ext2_mkdir,
		.utimes = ext2_utimes,
		.rmdir = ext2_rmdir,
		.link = ext2_link,
		.readlink = ext2_readlink,
	};
	vfs_mount("/", NULL, "/dev/ide1p1", "ext2", &cb);
}

#endif /* ENABLE_EXT2 */
