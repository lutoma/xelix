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
#include <fs/vfs.h>
#include <fs/ext2.h>

uint32_t ext2_resolve_inode(const char* path, uint32_t* parent_ino) {
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
	struct inode* inode = kmalloc(superblock->inode_size);
	struct dirent* dirent = NULL;
	uint32_t result = 0;

	while(pch != NULL) {
		int inode_num = dirent ? dirent->inode : ROOT_INODE;
		if(!ext2_inode_read(inode, inode_num)) {
			goto bye;
		}

		if(dirent) {
			kfree(dirent);
		}

		if(parent_ino) {
			*parent_ino = inode_num;
		}

		dirent = ext2_dirent_find(inode, pch);
		if(!dirent) {
			goto bye;
		}
		pch = strtok_r(NULL, "/", &sp);
	}
	result = dirent->inode;
bye:
	kfree(path_tmp);
	kfree(inode);
	return result;
}

static uint32_t handle_symlink(struct inode* inode, const char* path, uint32_t flags, void* mount_instance) {
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
		char* base_path = ext2_chop_path(path, NULL);
		new_path = vfs_normalize_path(sym_path, base_path);
		kfree(base_path);
	} else {
		new_path = strdup(sym_path);
	}

	// FIXME Should be vfs_open to make symlinks across mount points possible
	uint32_t r = ext2_open(new_path, flags, mount_instance);
	kfree(new_path);
	return r;
}

// The public open interface to the virtual file system
uint32_t ext2_open(char* path, uint32_t flags, void* mount_instance) {
	if(!path || !strcmp(path, "")) {
		log(LOG_ERR, "ext2: ext2_read_file called with empty path.\n");
		return 0;
	}

	uint32_t dir_inode = 0;
	uint32_t inode_num = ext2_resolve_inode(path, &dir_inode);
	struct inode* inode = kmalloc(superblock->inode_size);
	bool created = false;

	if(!inode_num) {
		if(!(flags & O_CREAT)) {
			kfree(inode);
			sc_errno = ENOENT;
			return 0;
		}

		debug("ext2_open: Could not find inode, creating one.\n");
		inode_num = ext2_inode_new(inode, FT_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		ext2_dirent_add(dir_inode, inode_num, vfs_basename(path), (uint8_t)FT_IFREG);
		created = true;
	} else {
		if((flags & O_CREAT) && (flags & O_EXCL)) {
			kfree(inode);
			sc_errno = EEXIST;
			return 0;
		}

		if(!ext2_inode_read(inode, inode_num)) {
			kfree(inode);
			sc_errno = ENOENT;
			return 0;
		}
	}

	uint32_t ft = vfs_mode_to_filetype(inode->mode);
	if(ft == FT_IFDIR && (flags & O_WRONLY || flags & O_RDWR)) {
		inode_num = 0;
		sc_errno = EISDIR;
	}

	if(!created && ft == FT_IFLNK) {
		inode_num = handle_symlink(inode, path, flags, mount_instance);
	}

	kfree(inode);
	return inode_num;
}

#endif /* ENABLE_EXT2 */
