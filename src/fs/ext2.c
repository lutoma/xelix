/* ext2.c: ext2 VFS callbacks
 * Copyright © 2013-2023 Lukas Martini
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
#include "ext2_misc.h"
#include "ext2_inode.h"
#include "ext2_dirent.h"
#include <log.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <libgen.h>
#include <mem/kmalloc.h>
#include <fs/vfs.h>
#include <fs/mount.h>
#include <block/block.h>

static vfs_file_t* ext2_open(struct vfs_callback_ctx* ctx, uint32_t flags);

static struct inode* get_inode_and_check_owner(struct ext2_fs* fs, struct vfs_callback_ctx* ctx, struct dirent** dirent) {
	*dirent = ext2_dirent_find(fs, ctx->path, NULL, ctx->task);
	if(!*dirent) {
		sc_errno = ENOENT;
		return NULL;
	}

	struct inode* inode = kmalloc(fs->superblock->inode_size);
	if(!ext2_inode_read(fs, inode, (*dirent)->inode)) {
		kfree(*dirent);
		kfree(inode);
		sc_errno = ENOENT;
		return NULL;
	}

	if(inode->uid != ctx->task->euid) {
		kfree(inode);
		sc_errno = EPERM;
		return NULL;
	}

	return inode;
}

static int ext2_chmod(struct vfs_callback_ctx* ctx, uint32_t mode) {
	struct dirent* dirent = NULL;
	struct ext2_fs* fs = ctx->mp->instance;
	struct inode* inode = get_inode_and_check_owner(fs, ctx, &dirent);
	if(!inode) {
		return -1;
	}

	inode->mode = vfs_mode_to_filetype(inode->mode) | (mode & 0xfff);
	ext2_inode_write(fs, inode, dirent->inode);
	kfree(dirent);
	kfree(inode);
	return 0;
}

static int ext2_chown(struct vfs_callback_ctx* ctx, uint16_t uid, uint16_t gid) {
	struct dirent* dirent = NULL;
	struct ext2_fs* fs = ctx->mp->instance;
	struct inode* inode = get_inode_and_check_owner(fs, ctx, &dirent);
	if(!inode) {
		return -1;
	}

	if(uid != -1) {
		inode->uid = uid;
	}
	if(gid != -1) {
		inode->gid = gid;
	}
	ext2_inode_write(fs, inode, dirent->inode);
	kfree(dirent);
	kfree(inode);
	return 0;
}

static int ext2_stat(struct vfs_callback_ctx* ctx, vfs_stat_t* dest) {
	struct ext2_fs* fs = ctx->mp->instance;
	struct dirent* dirent = ext2_dirent_find(fs, ctx->path, NULL, ctx->task);
	if(!dirent) {
		sc_errno = ENOENT;
		return -1;
	}

	struct inode* inode = kmalloc(fs->superblock->inode_size);
	if(!ext2_inode_read(fs, inode, dirent->inode)) {
		kfree(dirent);
		kfree(inode);
		sc_errno = ENOENT;
		return -1;
	}

	dest->st_dev = fs->dev->number;
	dest->st_ino = dirent->inode;
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

	kfree(dirent);
	kfree(inode);
	return 0;
}

static int ext2_mkdir(struct vfs_callback_ctx* ctx, uint32_t mode) {
	struct ext2_fs* fs = ctx->mp->instance;

	// Duplicate path as basedir() is destructive
	char* dup_path = strdup(ctx->path);
	if(!dup_path) {
		return -1;
	}

	const char* base_path = dirname(dup_path);

	// Ensure parent directory exists and permissions are ok
	struct dirent* parent = ext2_dirent_find(fs, base_path, NULL, ctx->task);
	kfree(dup_path);
	if(!parent) {
		sc_errno = ENOENT;
		return -1;
	}

	struct inode* parent_inode = kmalloc(fs->superblock->inode_size);
	if(!ext2_inode_read(fs, parent_inode, parent->inode)) {
		kfree(parent);
		kfree(parent_inode);
		sc_errno = ENOENT;
		return -1;
	}

	if(ext2_inode_check_perm(PERM_CHECK_WRITE, parent_inode, ctx->task) < 0) {
		kfree(parent);
		kfree(parent_inode);
		sc_errno = EACCES;
		return -1;
	}
	kfree(parent_inode);

	// Ensure directory doesn't already exist
	struct dirent* check_dirent = ext2_dirent_find(fs, ctx->path, NULL, ctx->task);
	if(check_dirent) {
		kfree(check_dirent);
		sc_errno = EEXIST;
		return -1;
	}

	struct inode* inode = kmalloc(fs->superblock->inode_size);
	uint32_t inode_num = ext2_inode_new(fs, inode, FT_IFDIR | S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

	// Create empty dirent block
	struct dirent* buf = (struct dirent*)zmalloc(bl_off(1));
	buf->inode = 0;
	buf->name_len = 0;
	buf->record_len = bl_off(1);

	if(!ext2_inode_write_data(fs, inode, inode_num, 0, sizeof(struct dirent), (uint8_t*)buf)) {
		kfree(parent);
		kfree(buf);
		sc_errno = ENOENT;
		return -1;
	}
	kfree(buf);

	inode->size = bl_off(1);
	if(ctx->task) {
		inode->uid = ctx->task->euid;
		inode->gid = ctx->task->egid;
	} else {
		inode->uid = 0;
		inode->gid = 0;
	}

	ext2_inode_write(fs, inode, inode_num);
	ext2_dirent_add(fs, parent->inode, inode_num, basename(ctx->path), EXT2_DIRENT_FT_DIR);

	// Add . and .. dirents
	ext2_dirent_add(fs, inode_num, inode_num, ".", EXT2_DIRENT_FT_DIR);
	ext2_dirent_add(fs, inode_num, parent->inode, "..", EXT2_DIRENT_FT_DIR);

	uint32_t blockgroup_num = inode_to_blockgroup(inode_num);
	fs->blockgroup_table[blockgroup_num].used_directories++;
	write_blockgroup_table();

	kfree(parent);
	kfree(inode);
	return 0;
}

static int ext2_utimes(struct vfs_callback_ctx* ctx, struct timeval times[2]) {
	struct ext2_fs* fs = ctx->mp->instance;
	struct dirent* dirent = ext2_dirent_find(fs, ctx->path, NULL, ctx->task);
	if(!dirent) {
		sc_errno = ENOENT;
		return -1;
	}

	struct inode* inode = kmalloc(fs->superblock->inode_size);
	if(!ext2_inode_read(fs, inode, dirent->inode)) {
		kfree(inode);
		kfree(dirent);
		sc_errno = ENOENT;
		return -1;
	}

	if(ext2_inode_check_perm(PERM_CHECK_WRITE, inode, ctx->task) < 0) {
		kfree(inode);
		kfree(dirent);
		sc_errno = EACCES;
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

	ext2_inode_write(fs, inode, dirent->inode);
	kfree(inode);
	kfree(dirent);
	return 0;
}

static int do_unlink(struct ext2_fs* fs, char* path, bool is_dir, task_t* task) {
	uint32_t dir_ino = 0;
	struct dirent* dirent = ext2_dirent_find(fs, path, &dir_ino, task);
	if(!dirent || !dir_ino) {
		sc_errno = ENOENT;
		return -1;
	}

	if(dirent->inode == ROOT_INODE) {
		kfree(dirent);
		sc_errno = EACCES;
		return -1;
	}

	struct inode* inode = kmalloc(fs->superblock->inode_size);
	if(!ext2_inode_read(fs, inode, dirent->inode)) {
		kfree(inode);
		kfree(dirent);
		sc_errno = ENOENT;
		return -1;
	}

	if(ext2_inode_check_perm(PERM_CHECK_WRITE, inode, task) < 0) {
		kfree(inode);
		kfree(dirent);
		sc_errno = EACCES;
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
			kfree(inode);
			kfree(dirent);
			return -1;
		}

		// FIXME Should probably check more throroughly it only contains ./..
		// and has no hard links
		if(link_count > 2) {
			kfree(inode);
			kfree(dirent);
			sc_errno = ENOTEMPTY;
			return -1;
		}

		// . and .. inside directory
		link_count -= 2;
	} else {
		if(vfs_mode_to_filetype(inode->mode) == FT_IFDIR) {
			kfree(inode);
			kfree(dirent);
			sc_errno = EISDIR;
			return -1;
		}
	}

	ext2_dirent_rm(fs, dir_ino, basename(path));
	link_count--;
	inode->link_count--;

	if(link_count < 1) {
		debug("do_unlink: inode %d link count < 1, purging.\n", dirent->inode);
		inode->dtime = time_get();
		inode->link_count = 0;

		// Free data blocks
		struct blockgroup* blockgroup = fs->blockgroup_table + inode_to_blockgroup(dirent->inode);
		struct ext2_blocknum_resolver_cache* res_cache = zmalloc(sizeof(struct ext2_blocknum_resolver_cache));

		for(int i = 0; true; i++) {
			uint32_t num = ext2_resolve_blocknum(fs, inode, i, res_cache);
			if(!num) {
				break;
			}

			ext2_bitmap_free(fs, blockgroup->block_bitmap, num % fs->superblock->blocks_per_group);
		}

		ext2_free_blocknum_resolver_cache(res_cache);
		ext2_bitmap_free(fs, blockgroup->inode_bitmap, (dirent->inode - 1) % fs->superblock->inodes_per_group);
		fs->superblock->free_inodes++;
		blockgroup->free_inodes++;

		// inode->block_count counts IDE blocks, free_blocks counts ext2 blocks.
		fs->superblock->free_blocks += inode->block_count / 8;
		blockgroup->free_blocks += inode->block_count / 8;

		if(is_dir) {
			blockgroup->used_directories--;

			// Decrease parent directory link count (removed .. entry)
			struct inode* dir_inode = kmalloc(fs->superblock->inode_size);
			if(!ext2_inode_read(fs, dir_inode, dir_ino)) {
				kfree(dir_inode);
				kfree(inode);
				kfree(dirent);
				sc_errno = ENOENT;
				return -1;
			}

			dir_inode->link_count--;
			ext2_inode_write(fs, dir_inode, dir_ino);
			kfree(dir_inode);
		}

		write_superblock();
		write_blockgroup_table();
	}

	ext2_inode_write(fs, inode, dirent->inode);
	kfree(inode);
	kfree(dirent);
	return 0;
}

static int ext2_unlink(struct vfs_callback_ctx* ctx) {
	struct ext2_fs* fs = ctx->mp->instance;
	return do_unlink(fs, ctx->path, false, ctx->task);
}

static int ext2_rmdir(struct vfs_callback_ctx* ctx) {
	struct ext2_fs* fs = ctx->mp->instance;
	return do_unlink(fs, ctx->path, true, ctx->task);
}

static int ext2_link(struct vfs_callback_ctx* ctx, const char* new_path) {
	struct ext2_fs* fs = ctx->mp->instance;

	// Duplicate path as basedir() is destructive
	char* dup_path = strdup(new_path);
	if(!dup_path) {
		return -1;
	}

	char* new_name = basename(dup_path);
	char* new_dir_path = dirname(dup_path);

	struct dirent* dirent = ext2_dirent_find(fs, ctx->path, NULL, ctx->task);
	struct dirent* dir_dirent = ext2_dirent_find(fs, new_dir_path, NULL, ctx->task);
	kfree(dup_path);

	if(!dirent || !dir_dirent) {
		kfree(dirent);
		kfree(dir_dirent);
		sc_errno = ENOENT;
		return -1;
	}

	// Directory hard links are not allowed in ext2
	if(dirent->type == EXT2_DIRENT_FT_DIR) {
		kfree(dirent);
		kfree(dir_dirent);
		sc_errno = EACCES;
		return -1;
	}

	ext2_dirent_add(fs, dir_dirent->inode, dirent->inode, new_name, dirent->type);
	kfree(dirent);
	kfree(dir_dirent);
	return 0;
}

static int ext2_access(struct vfs_callback_ctx* ctx, uint32_t amode) {
	struct ext2_fs* fs = ctx->mp->instance;
	struct dirent* dirent = ext2_dirent_find(fs, ctx->path, NULL, ctx->task);
	if(!dirent) {
		sc_errno = ENOENT;
		return -1;
	}

	struct inode* inode = kmalloc(fs->superblock->inode_size);
	if(!ext2_inode_read(fs, inode, dirent->inode)) {
		kfree(inode);
		kfree(dirent);
		sc_errno = ENOENT;
		return -1;
	}

	int perm_check = 0;
	if(amode & R_OK) {
		perm_check += ext2_inode_check_perm(PERM_CHECK_READ, inode, ctx->task);
	}
	if(amode & W_OK) {
		perm_check += ext2_inode_check_perm(PERM_CHECK_WRITE, inode, ctx->task);
	}
	if(amode & X_OK) {
		perm_check += ext2_inode_check_perm(PERM_CHECK_EXEC, inode, ctx->task);
	}
	if(perm_check < 0) {
		kfree(inode);
		kfree(dirent);
		sc_errno = EACCES;
		return -1;
	}

	kfree(inode);
	kfree(dirent);
	return 0;
}

static int ext2_readlink(struct vfs_callback_ctx* ctx, char* buf, size_t size) {
	struct ext2_fs* fs = ctx->mp->instance;
	struct dirent* dirent = ext2_dirent_find(fs, ctx->path, NULL, ctx->task);
	if(!dirent) {
		sc_errno = ENOENT;
		return -1;
	}

	struct inode* inode = kmalloc(fs->superblock->inode_size);
	if(!ext2_inode_read(fs, inode, dirent->inode)) {
		kfree(inode);
		kfree(dirent);
		sc_errno = ENOENT;
		return -1;
	}

	if(ext2_inode_check_perm(PERM_CHECK_READ, inode, ctx->task) < 0) {
		kfree(inode);
		kfree(dirent);
		sc_errno = EACCES;
		return -1;
	}

	if(vfs_mode_to_filetype(inode->mode) != FT_IFLNK) {
		kfree(inode);
		kfree(dirent);
		sc_errno = EINVAL;
		return -1;
	}

	size_t len = strlcpy(buf, (char*)inode->blocks, size);
	kfree(inode);
	kfree(dirent);
	return len;
}


static size_t ext2_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	struct ext2_fs* fs = ctx->mp->instance;
	if(!ctx->fp || !ctx->fp->inode || !fs) {
		log(LOG_ERR, "ext2: ext2_read_file called without file system struct, fp or inode.\n");
		sc_errno = EBADF;
		return -1;
	}

	debug("ext2_read_file for %s, off %d, size %d\n", ctx->fp->mount_path, ctx->fp->offset, size);

	struct inode* inode = kmalloc(fs->superblock->inode_size);
	if(!ext2_inode_read(fs, inode, ctx->fp->inode)) {
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

	uint8_t* read = ext2_inode_read_data(fs, inode, ctx->fp->offset, size, dest);
	kfree(inode);

	if(!read) {
		return 0;
	}
	return size;
}

static size_t ext2_write(struct vfs_callback_ctx* ctx, void* source, size_t size) {
	struct ext2_fs* fs = ctx->mp->instance;
	if(!ctx || !ctx->fp || !ctx->fp->inode) {
		log(LOG_ERR, "ext2: ext2_write_file called without fp or fp missing inode.\n");
		sc_errno = EBADF;
		return -1;
	}

	debug("ext2_write_file for %s, off %d, size %d\n", ctx->fp->mount_path, ctx->fp->offset, size);

	struct inode* inode = kmalloc(fs->superblock->inode_size);
	if(!ext2_inode_read(fs, inode, ctx->fp->inode)) {
		kfree(inode);
		sc_errno = EBADF;
		return -1;
	}

	if(ext2_inode_check_perm(PERM_CHECK_WRITE, inode, ctx->task) < 0) {
		kfree(inode);
		sc_errno = EACCES;
		return -1;
	}

	debug("%s uid=%d, gid=%d, size=%d, ft=%s mode=%s\n", ctx->fp->mount_path, inode->uid,
		inode->gid, inode->size, vfs_filetype_to_verbose(vfs_mode_to_filetype(inode->mode)),
		vfs_get_verbose_permissions(inode->mode));

	if(vfs_mode_to_filetype(inode->mode) != FT_IFREG) {
		debug("ext2_write_file: Attempt to write to something weird "
			"(0x%x: %s)\n", inode->mode,
			vfs_filetype_to_verbose(vfs_mode_to_filetype(inode->mode)));

		kfree(inode);
		sc_errno = EISDIR;
		return -1;
	}

	if(!ext2_inode_write_data(fs, inode, ctx->fp->inode, ctx->fp->offset, size, source)) {
		kfree(inode);
		return -1;
	}

	inode->size = ctx->fp->offset + size;
	inode->mtime = time_get();
	ext2_inode_write(fs, inode, ctx->fp->inode);
	kfree(inode);
	return size;
}


static size_t ext2_getdents(struct vfs_callback_ctx* ctx, void* buf, size_t size) {
	struct ext2_fs* fs = ctx->mp->instance;
	struct inode* inode = kmalloc(fs->superblock->inode_size);

	if(!ext2_inode_read(fs, inode, ctx->fp->inode)) {
		kfree(inode);
		sc_errno = EBADF;
		return -1;
	}

	if(ext2_inode_check_perm(PERM_CHECK_EXEC, inode, ctx->task) < 0) {
		kfree(inode);
		sc_errno = EACCES;
		return -1;
	}

	if(vfs_mode_to_filetype(inode->mode) != FT_IFDIR) {
		kfree(inode);
		sc_errno = ENOTDIR;
		return -1;
	}

	struct rd_r* rd_reent = zmalloc(sizeof(struct rd_r));
	uint64_t offset = 0;
	vfs_dirent_t* ent = NULL;

	while((ent = ext2_readdir_r(fs, inode, &ctx->fp->offset, rd_reent))) {
		if(offset + ent->d_reclen >= size) {
			ctx->fp->offset -= rd_reent->last_len;
			kfree(ent);
			break;
		}

		vfs_dirent_t* dest = (vfs_dirent_t*)(buf + offset);
		memcpy(dest, ent, ent->d_reclen);
		offset += ent->d_reclen;
		dest->d_off = offset;
		kfree(ent);
	}

	kfree(inode);
	kfree(rd_reent);
	return offset;
}

static int ext2_build_path_tree(struct vfs_callback_ctx* ctx) {
	struct ext2_fs* fs = ctx->mp->instance;
	struct dirent* dirent = ext2_dirent_find(fs, ctx->path, NULL, ctx->task);
	if(!dirent) {
		sc_errno = ENOENT;
		return -1;
	}
	return 0;
}


static vfs_file_t* handle_symlink(struct vfs_callback_ctx* ctx, struct inode* inode, uint32_t flags) {
	/* For symlinks with up to 60 chars length, the path is stored in the
	 * inode in the area where normally the block pointers would be.
	 * Otherwise in the file itself.
	 */
	if(inode->size > 60) {
		log(LOG_WARN, "ext2: Symlinks with length >60 are not supported right now.\n");
		kfree(inode);
		return NULL;
	}

	char* sym_path = (char*)inode->blocks;
	if(sym_path[0] != '/') {

		char* dup_path = strdup(ctx->path);
		if(!dup_path) {
			return NULL;
		}

		char* base_path = dirname(dup_path);
		ctx->path = vfs_normalize_path(sym_path, base_path);
		kfree(dup_path);
	} else {
		ctx->path = strdup(sym_path);
	}

	// FIXME Should be vfs_open to make symlinks across mount points possible
	vfs_file_t* r = ext2_open(ctx, flags);
	return r;
}

static vfs_file_t* ext2_open(struct vfs_callback_ctx* ctx, uint32_t flags) {
	struct ext2_fs* fs = ctx->mp->instance;
	if(!ctx->path || !strcmp(ctx->path, "")) {
		sc_errno = EINVAL;
		return NULL;
	}

	// Duplicate path as basedir() is destructive
	char* dup_path = strdup(ctx->path);
	if(!dup_path) {
		return NULL;
	}

	const char* parent_dir = dirname(dup_path);

	// Look up the parent directory first and fail if it cannot be found.
	// This is important to ensure files can only be created in existing
	// directories. below
	struct dirent* parent_dirent = ext2_dirent_find(fs, parent_dir, NULL, ctx->task);
	kfree(dup_path);
	if(!parent_dirent || !parent_dirent->inode) {
		sc_errno = ENOENT;
		return NULL;
	}

	uint32_t parent_inode = parent_dirent->inode;
	kfree(parent_dirent);

	// FIXME Most of this just resolves the same path as above. Should take a
	// shortcut here / reuse existing data with ext2_dirent_search
	struct dirent* dirent = ext2_dirent_find(fs, ctx->path, NULL, ctx->task);
	if(!dirent && (sc_errno != ENOENT || !(flags & O_CREAT))) {
		sc_errno = ENOENT;
		return NULL;
	}

	uint32_t inode_num;
	struct inode* inode = kmalloc(fs->superblock->inode_size);

	if(!dirent || !dirent->inode) {
		inode_num = ext2_inode_new(fs, inode, FT_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
		ext2_dirent_add(fs, parent_inode, inode_num, basename(ctx->path), EXT2_DIRENT_FT_REG_FILE);

		if(ctx->task) {
			inode->uid = ctx->task->euid;
			inode->gid = ctx->task->egid;
		}
	} else {
		inode_num = dirent->inode;
		kfree(dirent);

		if((flags & O_CREAT) && (flags & O_EXCL)) {
			kfree(inode);
			sc_errno = EEXIST;
			return NULL;
		}

		if(!ext2_inode_read(fs, inode, inode_num)) {
			kfree(inode);
			sc_errno = ENOENT;
			return NULL;
		}

		if((flags & O_WRONLY) || (flags & O_RDWR)) {
			if(ext2_inode_check_perm(PERM_CHECK_WRITE, inode, ctx->task) < 0) {
				kfree(inode);
				sc_errno = EACCES;
				return NULL;
			}
		}
	}

	uint16_t ft = vfs_mode_to_filetype(inode->mode);
	if(ft == FT_IFDIR && (flags & O_WRONLY || flags & O_RDWR)) {
		kfree(inode);
		sc_errno = EISDIR;
		return NULL;
	}

	if(ft == FT_IFLNK) {
		vfs_file_t* r = handle_symlink(ctx, inode, flags);
		kfree(inode);
		return r;
	}
	kfree(inode);

	vfs_file_t* fp = vfs_alloc_fileno(ctx->task, 3);
	if(!fp) {
		return NULL;
	}

	fp->type = ft;
	fp->inode = inode_num;
	memcpy(&fp->callbacks, fs->callbacks, sizeof(struct vfs_callbacks));

	sc_errno = 0;
	return fp;
}

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
		sc_errno = EINVAL;
		return -1;
	}

	if(unlikely(fs->superblock->revision != 1)) {
		log(LOG_ERR, "ext2: Only revision 1 is supported.\n");

		kfree(fs);
		sc_errno = EINVAL;
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
		sc_errno = EINVAL;
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
	vfs_mount_register(dev, path, (void*)fs, "ext2", &cb);
	return 0;
}

#endif /* CONFIG_ENABLE_EXT2 */
