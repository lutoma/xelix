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
#include <memory/kmalloc.h>
#include <hw/ide.h>
#include <hw/serial.h>
#include <fs/vfs.h>
#include <fs/ext2.h>


void ext2_insert_dirent(uint32_t dir_num, uint32_t inode_num, char* name, uint8_t type) {
	debug("ext2_new_dirent dir %d ino %d name %s\n", dir_num, inode_num, name);

	struct inode* dir = kmalloc(sizeof(struct inode));
	ext2_read_inode(dir, dir_num);

	if(dir->flags & EXT2_INDEX_FL) {
		log(LOG_ERR, "ext2_insert_dirent: No support for writing to indexed dirents.\n");
		kfree(dir);
		return;
	}

	void* dirents = kmalloc(dir->size);
	if(!ext2_read_inode_blocks(dir, dir->size / superblock_to_blocksize(superblock), dirents)) {
		return;
	}

	// Length/offsets need to be 4-aligned
	size_t dlen = sizeof(vfs_dirent_t) + strlen(name);
	if(dlen % 4) {
		dlen = (dlen & -4) + 4;
	}

	vfs_dirent_t* current_ent = dirents;
	while((void*)current_ent < dirents + dir->size) {
		if(!current_ent->inode) {
			goto next;
		}

		uint32_t free_space = current_ent->record_len - sizeof(vfs_dirent_t) - current_ent->name_len;
		if(free_space > dlen) {
			break;
		}

		next:
		current_ent = (vfs_dirent_t*)((intptr_t)current_ent + (intptr_t)current_ent->record_len);
	}

	vfs_dirent_t* new_dirent = kmalloc(dlen);
	bzero(new_dirent, dlen);

	new_dirent->inode = inode_num;
	new_dirent->record_len = dlen;
	new_dirent->name_len = strlen(name);
	memcpy((void*)new_dirent + sizeof(vfs_dirent_t), name, new_dirent->name_len);

	uint32_t new_len = current_ent->record_len - dlen;
	current_ent->record_len = sizeof(vfs_dirent_t) + current_ent->name_len;
	new_dirent->record_len = new_len;
	memcpy((void*)current_ent + current_ent->record_len, new_dirent, dlen);
	ext2_write_inode_blocks(dir, dir_num, dir->size / superblock_to_blocksize(superblock), dirents);

	// FIXME Update parent directory mtime/ctime
	kfree(new_dirent);
	kfree(dirents);
	kfree(dir);
}

size_t ext2_getdents(vfs_file_t* fp, void* dest, size_t size) {
	if(size % 1024) {
		log(LOG_ERR, "ext2: Size argument to ext2_getdents needs to be a multiple of 1024.\n");
		return 0;
	}

	if(!fp || !fp->inode) {
		log(LOG_ERR, "ext2: ext2_read_directory called without fp or fp missing inode.\n");
		return 0;
	}

	struct inode* inode = kmalloc(superblock->inode_size);
	if(!ext2_read_inode(inode, fp->inode)) {
		kfree(inode);
		return 0;
	}

	// Check if this inode is a directory
	if(vfs_mode_to_filetype(inode->mode) != FT_IFDIR)
	{
		debug("ext2_read_directory: This inode isn't a directory "
			"(Is %s [%d])\n", vfs_filetype_to_verbose(vfs_mode_to_filetype(inode->mode)),
				inode->mode);

		kfree(inode);
		return 0;
	}

	if(size > inode->size) {
		debug("ext2_read_file: Attempt to read 0x%x bytes, but file is only 0x%x bytes. Capping.\n", size, inode->size);
		size = inode->size;
	}

	if(!ext2_read_inode_blocks(inode, size / superblock_to_blocksize(superblock), dest)) {
		return 0;
	}

	kfree(inode);
	return size;
}

#endif /* ENABLE_EXT2 */
