/* ftree.c: VFS file tree
 * Copyright © 2019 Lukas Martini
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

#include <fs/ftree.h>
#include <string.h>
#include <mem/kmalloc.h>
#include <fs/vfs.h>
#include <fs/sysfs.h>
#include <kavl.h>

#define kavl_strcmp(p, q) (strcmp(p->path, q->path))
KAVL_INIT(my, struct ftree_file, head, kavl_strcmp)
static struct ftree_file* ftree_root = NULL;


struct ftree_file* vfs_ftree_insert(struct ftree_file* root, char* name, vfs_stat_t* stat) {
	struct ftree_file* p = zmalloc(sizeof(struct ftree_file));
	struct ftree_file* q = NULL;

	strncpy(p->path, name, ARRAY_SIZE(p->path) - 1);
	memcpy(&p->stat, stat, sizeof(vfs_stat_t));

	q = kavl_insert(my, &(root ? root : ftree_root)->children, p, 0);
	if(p != q) {
		// Duplicate
		kfree(p);
		memcpy(&q->stat, stat, sizeof(vfs_stat_t));
	}

	return q;
}

// FIXME should not exist - does not fill stat structs of path
struct ftree_file* vfs_ftree_insert_path(char* path, vfs_stat_t* stat) {
	char* sp;
	char* path_tmp = strndup(path, 500);
	char* pch = strtok_r(path_tmp, "/", &sp);

	struct ftree_file* cur = ftree_root;
	while(pch != NULL) {
		cur = vfs_ftree_insert(cur, pch, stat);
		pch = strtok_r(NULL, "/", &sp);
	}
	kfree(path_tmp);


//	return q;
	return NULL;
}

const struct ftree_file* vfs_ftree_find(const struct ftree_file* root, char* name) {
	kavl_itr_t(my) itr;
	struct ftree_file* in = kmalloc(sizeof(struct ftree_file));
	strncpy(in->path, name, ARRAY_SIZE(in->path) - 1);
	kavl_itr_find(my, (root ? root : ftree_root)->children, in, &itr);

	const struct ftree_file* p = kavl_at(&itr);
	if(!p || strcmp(in->path, p->path)) {
		kfree(in);
		return NULL;
	}

	return p;
}

const struct ftree_file* vfs_ftree_find_path(char* path) {
	if(!strcmp("/", path)) {
		return ftree_root;
	}

	char* sp;
	char* path_tmp = strndup(path, 500);
	char* pch = strtok_r(path_tmp, "/", &sp);
	const struct ftree_file* cur = ftree_root;

	while(pch != NULL) {
		cur = vfs_ftree_find(cur, pch);
		pch = strtok_r(NULL, "/", &sp);
	}

	kfree(path_tmp);
	return cur;
}

static size_t sfs_iter(struct vfs_callback_ctx* ctx, void* dest, size_t size,
	size_t rsize, const struct ftree_file* root, int depth) {

	kavl_itr_t(my) itr;
	kavl_itr_first(my, root->children, &itr);

	do {
		const struct ftree_file *p = kavl_at(&itr);
		sysfs_printf("%*s%-*s %-8d %-12d %s\n",
			depth * 2, "",
			50 - (depth * 2), p->path,
			p->stat.st_ino, p->stat.st_size,
			vfs_filetype_to_verbose(vfs_mode_to_filetype(p->stat.st_mode)));

		if(p && p->children && depth < 5) {
			rsize = sfs_iter(ctx, dest, size, rsize, p, depth + 1);
		}
	} while(kavl_itr_next(my, &itr));
	return rsize;
}

static size_t sfs_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	if(ctx->fp->offset) {
		return 0;
	}

	return sfs_iter(ctx, dest, size, 0, ftree_root, 0);
}

void vfs_ftree_init() {
	ftree_root = zmalloc(sizeof(struct ftree_file));
	strncpy(ftree_root->path, "/", ARRAY_SIZE(ftree_root->path));
	ftree_root->stat.st_dev = 1;
	ftree_root->stat.st_ino = 2;
	ftree_root->stat.st_mode = FT_IFDIR | S_IXUSR | S_IRUSR | S_IXGRP | S_IRGRP | S_IXOTH | S_IROTH;
	ftree_root->stat.st_nlink = 1;
	ftree_root->stat.st_blocks = 2;
	ftree_root->stat.st_blksize = 1024;
	ftree_root->stat.st_uid = 0;
	ftree_root->stat.st_gid = 0;
	ftree_root->stat.st_rdev = 0;
	ftree_root->stat.st_size = 0;
	uint32_t t = time_get();
	ftree_root->stat.st_atime = t;
	ftree_root->stat.st_mtime = t;
	ftree_root->stat.st_ctime = t;

	struct vfs_callbacks sfs_cb = {
		.read = sfs_read,
	};
	sysfs_add_file("vfs_ftree", &sfs_cb);
}
