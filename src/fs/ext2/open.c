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
#include <memory/kmalloc.h>
#include <hw/ide.h>
#include <hw/serial.h>
#include <fs/vfs.h>
#include <fs/ext2.h>

// The public open interface to the virtual file system
uint32_t ext2_open(char* path, void* mount_instance) {
	if(!path || !strcmp(path, "")) {
		log(LOG_ERR, "ext2: ext2_read_file called with empty path.\n");
		return 0;
	}

	debug("Resolving inode for path %s\n", path);

	// The root directory always has inode 2
	if(unlikely(!strcmp("/", path)))
		return ROOT_INODE;

	// Split path and iterate trough the single parts, going from / upwards.
	char* pch;
	char* sp;

	// Throwaway pointer for strtok_r
	char* path_tmp = strndup(path, 500);
	pch = strtok_r(path_tmp, "/", &sp);
	struct inode* current_inode = kmalloc(superblock->inode_size);
	vfs_dirent_t* dirent = NULL;
	uint8_t* dirent_block = NULL;

	while(pch != NULL)
	{
		if(!read_inode(current_inode, dirent ? dirent->inode : ROOT_INODE)) {
			continue;
		}

		if(dirent_block) {
			kfree(dirent_block);
		}

		dirent_block = kmalloc(current_inode->size);
		if(!read_inode_blocks(current_inode, current_inode->size / superblock_to_blocksize(superblock), dirent_block)) {
			kfree(dirent_block);
			kfree(path_tmp);
			kfree(current_inode);
			return 0;
		}

		dirent = (vfs_dirent_t*)dirent_block;

		// Now search the current inode for the searched directory part
		// TODO Maybe use a binary search or something similar here
		for(int i = 0;; i++)
		{
			// If this dirent is NULL, this means there are no more files
			if(!dirent || !dirent->name_len) {
				kfree(dirent_block);
				kfree(path_tmp);
				kfree(current_inode);
				return 0;
			}

			char* dirent_name = strndup(dirent->name, dirent->name_len);

			// Check if this is what we're searching for
			if(!strcmp(pch, dirent_name))
			{
				kfree(dirent_name);
				break;
			}

			kfree(dirent_name);
			dirent = ((vfs_dirent_t*)((intptr_t)dirent + dirent->record_len));
		}

		pch = strtok_r(NULL, "/", &sp);
	}

	uint32_t inode_num = dirent->inode;
	kfree(path_tmp);
	kfree(dirent_block);
	kfree(current_inode);

	// Handle symbolic links
	struct inode* inode = kmalloc(superblock->inode_size);
	if(!read_inode(inode, inode_num)) {
		return 0;
	}

	if(vfs_mode_to_filetype(inode->mode) == FT_IFLNK) {
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
		char* new_path;
		if(sym_path[0] != '/') {
			char* base_path = strdup(path);
			char* c = base_path + strlen(path);
			for(; c > path; c--) {
				if(*c == '/') {
					*c = 0;
					break;
				}
			}

			new_path = vfs_normalize_path(sym_path, base_path);
			kfree(base_path);
		} else {
			new_path = strdup(sym_path);
		}

		kfree(inode);

		// FIXME Should be vfs_open to make symlinks across mount points possible
		uint32_t r = ext2_open(new_path, mount_instance);
		kfree(new_path);
		return r;
	}

	kfree(inode);
	return inode_num;
}

#endif /* ENABLE_EXT2 */
