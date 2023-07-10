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

#ifdef CONFIG_ENABLE_EXT2

#include "ext2_internal.h"
#include "misc.h"
#include "inode.h"
#include "dirent.h"
#include <log.h>
#include <string.h>
#include <errno.h>
#include <mem/kmalloc.h>
#include <fs/vfs.h>
#include <fs/ftree.h>

#define dirent_off *offset - reent->read_off
vfs_dirent_t* ext2_readdir_r(struct ext2_fs* fs, struct inode* inode, uint64_t* offset, struct rd_r* reent) {
	while(1) {
		if(dirent_off + sizeof(struct dirent) >= reent->read_len) {
			if(*offset >= inode->size) {
				return NULL;
			}

			size_t rsize = MIN(sizeof(reent->buf), inode->size - *offset);
			if(!ext2_inode_read_data(fs, inode, *offset, rsize, reent->buf)) {
				return NULL;
			}
			reent->read_off = *offset;
			reent->read_len = rsize;
		}

		struct dirent* ent = (struct dirent*)(reent->buf + dirent_off);
		if(dirent_off + sizeof(struct dirent) + ent->name_len >= reent->read_len) {
			reent->read_len = 0;
			continue;
		}

		reent->last_len = ent->record_len;
		*offset += ent->record_len;
		if(!ent->inode) {
			continue;
		}

		// Convert ext2 dirent format to regular dirent
		size_t length = sizeof(vfs_dirent_t) + ent->name_len + 2;
		vfs_dirent_t* result = (vfs_dirent_t*)zmalloc(length);
		result->d_ino = ent->inode;
		result->d_type = ent->type;
		result->d_reclen = length;
		memcpy(result->d_name, ent->name, ent->name_len);
		result->d_name[ent->name_len] = 0;
		return result;
	}
}

// Looks for a directory entry with name `search` in a directory inode
static struct dirent* search_dir(struct ext2_fs* fs, struct inode* inode, const char* search) {
	struct dirent* result = NULL;
	struct rd_r* rd_reent = zmalloc(sizeof(struct rd_r));

	vfs_dirent_t* ent = NULL;
	uint64_t offset = 0;
	while((ent = ext2_readdir_r(fs, inode, &offset, rd_reent))) {
		if(!strcmp(ent->d_name, search)) {
			result = kmalloc(ent->d_reclen);
			memcpy(result, ent, ent->d_reclen);
			kfree(ent);
			break;
		}
		kfree(ent);
	}

	kfree(rd_reent);
	return result;
}

struct dirent* ext2_dirent_find(struct ext2_fs* fs, const char* path, uint32_t* parent_ino, task_t* task) {

	if(unlikely(!strcmp("/", path)))
		path = "/.";

	// Split path and iterate trough the single parts, going from / upwards.
	char* pch;
	char* sp;

	// Throwaway pointer for strtok_r
	char* path_tmp = strndup(path, 500);
	pch = strtok_r(path_tmp, "/", &sp);
	struct inode* inode = kmalloc(fs->superblock->inode_size);
	struct dirent* dirent = NULL;
	struct dirent* result = NULL;

	#ifdef CONFIG_ENABLE_FTREE
	struct ftree_file* ft_root = NULL;
	#endif

	if(unlikely(!ext2_inode_read(fs, inode, ROOT_INODE))) {
		goto bye;
	}

	while(pch != NULL) {
		int inode_num = dirent ? dirent->inode : ROOT_INODE;
		if(dirent) {
			kfree(dirent);
		}

		if(parent_ino) {
			*parent_ino = inode_num;
		}

		dirent = search_dir(fs, inode, pch);
		if(!dirent) {
			sc_errno = ENOENT;
			goto bye;
		}

		if(!ext2_inode_read(fs, inode, dirent->inode)) {
			sc_errno = ENOENT;
			goto bye;
		}

		if(ext2_inode_check_perm(PERM_CHECK_EXEC, inode, task) < 0) {
			sc_errno = EACCES;
			goto bye;
		}


		#ifdef CONFIG_ENABLE_FTREE
		vfs_stat_t stat = {
			.st_dev = 1,
			.st_ino = dirent->inode,
			.st_mode = inode->mode,
			.st_nlink = inode->link_count,
			.st_uid = inode->uid,
			.st_gid = inode->gid,
			.st_rdev = 0,
			.st_size = inode->size,
			.st_atime = inode->atime,
			.st_mtime = inode->mtime,
			.st_ctime = inode->ctime,
			.st_blksize = 512,
			.st_blocks = inode->block_count,
		};

		ft_root = vfs_ftree_insert(ft_root, pch, &stat);
		#endif

		pch = strtok_r(NULL, "/", &sp);
	}
	result = dirent;
bye:
	kfree(path_tmp);
	kfree(inode);
	return result;
}

void ext2_dirent_rm(struct ext2_fs* fs, uint32_t inode_num, char* name) {
	struct inode* inode = kmalloc(fs->superblock->inode_size);
	if(!ext2_inode_read(fs, inode, inode_num)) {
		kfree(inode);
		return;
	}

	uint8_t* dirent_block = kmalloc(inode->size);
	if(!ext2_inode_read_data(fs, inode, 0, inode->size, dirent_block)) {
		kfree(dirent_block);
		kfree(inode);
		return;
	}

	struct dirent* dirent = (struct dirent*)dirent_block;
	struct dirent* prev = NULL;
	bool found = false;

	while(dirent < (struct dirent*)(dirent_block + inode->size)) {
		if(!dirent->inode) {
			goto next;
		}

		// Check if this is what we're searching for
		char* dirent_name = strndup(dirent->name, dirent->name_len);
		if(!strcmp(name, dirent_name)) {
			kfree(dirent_name);
			found = true;
			break;
		}

		kfree(dirent_name);

		next:
		prev = dirent;
		dirent = ((struct dirent*)((intptr_t)dirent + dirent->record_len));
	}

	if(!found) {
		kfree(dirent_block);
		kfree(inode);
		return;
	}

	dirent->inode = 0;
	if(prev) {
		prev->record_len += dirent->record_len;
	}

	ext2_inode_write_data(fs, inode, inode_num, 0, inode->size, dirent_block);
	kfree(dirent_block);
	kfree(inode);
}

static inline uint32_t align_dirent_len(uint32_t dlen) {
	if(dlen <= 12) {
		return 12;
	}
	if(dlen % 4) {
		return (dlen & -4) + 4;
	}
	return dlen;
}

void ext2_dirent_add(struct ext2_fs* fs, uint32_t dir_num, uint32_t inode_num, char* name, uint8_t type) {
	debug("ext2_new_dirent dir %d ino %d name %s\n", dir_num, inode_num, name);

	struct inode* dir = kmalloc(fs->superblock->inode_size);
	if(!ext2_inode_read(fs, dir, dir_num)) {
		kfree(dir);
		return;
	}

	if(dir->flags & EXT2_INDEX_FL) {
		log(LOG_ERR, "ext2_dirent_add: No support for writing to indexed dirents.\n");
		kfree(dir);
		return;
	}

	void* dirents = kmalloc(dir->size);
	if(!ext2_inode_read_data(fs, dir, 0, dir->size, dirents)) {
		kfree(dir);
		kfree(dirents);
		return;
	}

	// Length/offsets need to be 4-aligned. +1 for NULL to terminate name
	size_t dlen = align_dirent_len(sizeof(struct dirent) + strlen(name) + 1);

	// Cycle through dirents until we find one with enough space to insert ours.
	struct dirent* current_ent = dirents;
	bool recycled_dirent = false;

	while(current_ent < (struct dirent*)(dirents + dir->size)) {
		uint32_t free_space;
		if(current_ent->inode) {
			free_space = current_ent->record_len - align_dirent_len(sizeof(struct dirent) + current_ent->name_len);
			recycled_dirent = false;
		} else {
			// Dirents with inode 0 are unused, we can recycle the whole of it
			free_space = current_ent->record_len;
			recycled_dirent = true;
		}

		if(free_space > dlen) {
			break;
		}

		current_ent = (struct dirent*)((intptr_t)current_ent + (intptr_t)current_ent->record_len);
	}

	struct dirent* new_dirent = recycled_dirent ? current_ent : zmalloc(dlen);
	new_dirent->inode = inode_num;
	new_dirent->name_len = strlen(name);
	new_dirent->type = type;
	memcpy((void*)new_dirent + sizeof(struct dirent), name, new_dirent->name_len);

	if(!recycled_dirent) {
		uint32_t old_len = current_ent->record_len;
		current_ent->record_len = align_dirent_len(sizeof(struct dirent) + current_ent->name_len);
		new_dirent->record_len = old_len - current_ent->record_len;
		memcpy((void*)current_ent + current_ent->record_len, new_dirent, dlen);
	}

	ext2_inode_write_data(fs, dir, dir_num, 0, dir->size, dirents);

	// Increase inode link count
	struct inode* inode = kmalloc(fs->superblock->inode_size);
	if(ext2_inode_read(fs, inode, inode_num)) {
		inode->link_count++;
		ext2_inode_write(fs, inode, inode_num);
	}

	// FIXME Update parent directory mtime/ctime

	if(!recycled_dirent) {
		kfree(new_dirent);
	}

	kfree(dirents);
	kfree(dir);
}

#endif /* CONFIG_ENABLE_EXT2 */
