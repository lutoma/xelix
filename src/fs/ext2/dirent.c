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
#include <memory/kmalloc.h>
#include <hw/ide.h>
#include <hw/serial.h>
#include <fs/vfs.h>
#include <fs/ext2.h>

static struct dirent* readdir_r(vfs_file_t* fp, void* kbuf, size_t size, size_t block_offset) {
	struct dirent* ent = kbuf + block_offset;

	if(block_offset > size || block_offset + sizeof(struct dirent) + ent->name_len > size) {
		return NULL;
	}

	fp->offset += ent->record_len;
	return ent;
}

size_t ext2_getdents(vfs_file_t* fp, void* buf, size_t size) {
	void* kbuf = kmalloc(size);
	size_t read = ext2_do_read(fp, kbuf, size, FT_IFDIR);
	if(!read) {
		kfree(kbuf);
		return 0;
	}

	uint32_t offset = 0;
	vfs_dirent_t* ent = NULL;
	struct dirent* ext2_ent = NULL;
	size_t orig_ofs = fp->offset;

	while(1) {
		ext2_ent = readdir_r(fp, kbuf, size, (fp->offset - orig_ofs));
		if(!ext2_ent)  {
			break;
		}

		uint16_t reclen = sizeof(vfs_dirent_t) + ext2_ent->name_len + 2;
		if(offset + reclen > size) {
			break;
		}

		//char* dname = strndup(ext2_ent->name, ext2_ent->name_len);
		//serial_printf("name %s record len %d inode %d\n", dname, ext2_ent->record_len, ext2_ent->inode);
		if(!ext2_ent->inode) {
			goto next;
		}

		ent = (vfs_dirent_t*)(buf + offset);
		memcpy(ent->d_name, ext2_ent->name, ext2_ent->name_len);
		ent->d_name[ext2_ent->name_len] = 0;
		ent->d_ino = ext2_ent->inode;
		ent->d_type = ext2_ent->type;
		ent->d_reclen = reclen;

		offset += ent->d_reclen;
		ent->d_off = offset;

		next:
		ext2_ent = (struct dirent*)((intptr_t)ext2_ent + (intptr_t)ext2_ent->record_len);
	}

	if(ext2_ent && offset < size) {
		ent->d_off += size - offset;
		ent->d_reclen += size - offset;
		offset = size;
	}

	kfree(kbuf);
	return offset;
}

struct dirent* ext2_dirent_find(struct inode* inode, const char* search) {
	uint8_t* dirent_block = kmalloc(inode->size);
	if(!ext2_inode_read_data(inode, 0, inode->size, dirent_block)) {
		return NULL;
	}

	struct dirent* dirent = (struct dirent*)dirent_block;
	while(dirent < (struct dirent*)(dirent_block + inode->size)) {
		if(!dirent->inode) {
			goto next;
		}

		// Check if this is what we're searching for
		char* dirent_name = strndup(dirent->name, dirent->name_len);
		if(!strcmp(search, dirent_name)) {
			kfree(dirent_name);

			struct dirent* rdir = kmalloc(dirent->record_len);
			memcpy(rdir, dirent, dirent->record_len);
			kfree(dirent_block);
			return rdir;
		}

		kfree(dirent_name);

		next:
		dirent = ((struct dirent*)((intptr_t)dirent + dirent->record_len));
	}

	kfree(dirent_block);
	return NULL;
}

void ext2_dirent_rm(uint32_t inode_num, char* name) {
	struct inode* inode = kmalloc(sizeof(struct inode));
	if(!ext2_inode_read(inode, inode_num)) {
		kfree(inode);
		return;
	}

	uint8_t* dirent_block = kmalloc(inode->size);
	if(!ext2_inode_read_data(inode, 0, inode->size, dirent_block)) {
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
		return;
	}

	dirent->inode = 0;
	if(prev) {
		prev->record_len += dirent->record_len;
	}

	ext2_inode_write_data(inode, inode_num, 0, inode->size, dirent_block);
	kfree(dirent_block);
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

void ext2_dirent_add(uint32_t dir_num, uint32_t inode_num, char* name, uint8_t type) {
	debug("ext2_new_dirent dir %d ino %d name %s\n", dir_num, inode_num, name);

	struct inode* dir = kmalloc(sizeof(struct inode));
	if(!ext2_inode_read(dir, dir_num)) {
		kfree(dir);
		return;
	}

	if(dir->flags & EXT2_INDEX_FL) {
		log(LOG_ERR, "ext2_dirent_add: No support for writing to indexed dirents.\n");
		kfree(dir);
		return;
	}

	void* dirents = kmalloc(dir->size);
	if(!ext2_inode_read_data(dir, 0, dir->size, dirents)) {
		kfree(dir);
		kfree(dirents);
		return;
	}

	// Length/offsets need to be 4-aligned
	size_t dlen = align_dirent_len(sizeof(struct dirent) + strlen(name));

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
	}

	memcpy((void*)current_ent + current_ent->record_len, new_dirent, dlen);
	ext2_inode_write_data(dir, dir_num, 0, dir->size, dirents);

	// Increase inode link count
	struct inode* inode = kmalloc(sizeof(struct inode));
	if(ext2_inode_read(inode, inode_num)) {
		inode->link_count++;
		ext2_inode_write(inode, inode_num);
	}

	// FIXME Update parent directory mtime/ctime

	if(!recycled_dirent) {
		kfree(new_dirent);
	}

	kfree(dirents);
	kfree(dir);
}

#endif /* ENABLE_EXT2 */
