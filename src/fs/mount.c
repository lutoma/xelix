/* mount.c: File system mount points
 * Copyright © 2011-2020 Lukas Martini
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

#include <fs/mount.h>
#include <fs/vfs.h>
#include <block/block.h>
#include <fs/sysfs.h>
#include <fs/ext2/ext2.h>
#include <tasks/task.h>
#include <mem/kmalloc.h>
#include <errno.h>
#include <panic.h>
#include <stdbool.h>

static struct vfs_mountpoint* mountpoints;

int vfs_mount_register(struct vfs_block_dev* dev, const char* path,
	void* instance, const char* type, struct vfs_callbacks* callbacks) {

	if(dev) {
		dev->mounted = true;
	}

	struct vfs_mountpoint* mp = kmalloc(sizeof(struct vfs_mountpoint));
	strcpy(mp->type, type);
	strcpy(mp->path, path);
	memcpy(&mp->callbacks, callbacks, sizeof(struct vfs_callbacks));

	mp->instance = instance;
	mp->dev = dev;
	mp->prev = NULL;
	mp->next = mountpoints;
	if(mp->next) {
		mp->next->prev = mp;
	}

	mountpoints = mp;

	if(dev) {
		log(LOG_DEBUG, "vfs: Mounted /dev/%s (type %s) to %s\n", dev->name, type, path);
	} else {
		log(LOG_DEBUG, "vfs: Mounted %s (type %s) to %s\n", type, type, path);
	}
	return 0;
}

struct vfs_mountpoint* vfs_mount_get(const char* path, char** mount_path) {
	struct vfs_mountpoint* match = NULL;
	size_t match_len = 0;
	const char* mpath = NULL;

	struct vfs_mountpoint* cur_mp = mountpoints;
	for(; cur_mp; cur_mp = cur_mp->next) {
		size_t plen = strlen(cur_mp->path);

		if(!strncmp(path, cur_mp->path, plen - 1) && plen > match_len) {
			match = cur_mp;
			match_len = plen;

			if(strlen(path) - plen > 0) {
				mpath = path + plen;
			} else {
				mpath = "/";
			}
		}
	}

	if(mount_path && mpath) {
		*mount_path = strdup(mpath);
	}
	return match;
}

int vfs_mount(task_t* task, const char* source, const char* target, int flags) {
	if(task && task->euid != 0) {
		sc_errno = EPERM;
		return -1;
	}

	char* mnt_target;
	if(!strcmp(target, "/")) {
		mnt_target = strdup("/");
	} else {
		struct vfs_callback_ctx* ctx = vfs_context_from_path(target, task);
		if(!ctx) {
			sc_errno = EBADF;
			return -1;
		}
		mnt_target = strdup(ctx->orig_path);

		if(!ctx->mp->callbacks.stat) {
			vfs_free_context(ctx);
			sc_errno = ENOSYS;
			return -1;
		}

		vfs_stat_t stat;
		if(ctx->mp->callbacks.stat(ctx, &stat) < 0) {
			vfs_free_context(ctx);
			return -1;
		}

		vfs_free_context(ctx);
		if(!(stat.st_mode & FT_IFDIR)) {
			sc_errno = ENOTDIR;
			return -1;
		}
	}

	struct vfs_block_dev* dev = vfs_block_get_dev(source);
	if(!dev) {
		kfree(mnt_target);
		sc_errno = ENOENT;
		return -1;
	}

	if(dev->mounted) {
		kfree(mnt_target);
		sc_errno = EBUSY;
		return -1;
	}

	int r = -1;
	#ifdef CONFIG_ENABLE_EXT2
	r = ext2_mount(dev, mnt_target);
	#endif

	kfree(mnt_target);
	return r;
}

int vfs_umount(task_t* task, const char* target, int flags) {
	if(task->euid != 0) {
		sc_errno = EPERM;
		return -1;
	}

	struct vfs_mountpoint* mp =  vfs_mount_get(target, NULL);
	if(!mp) {
		sc_errno = EINVAL;
		return -1;
	}

	if(!strcmp(mp->path, "/")) {
		sc_errno = EBUSY;
		return -1;
	}

	if(mp->prev) {
		mp->prev->next = mp->next;
	}

	if(mp->next) {
		mp->next->prev = mp->prev;
	}

	if(mountpoints == mp) {
		mountpoints = mp->next;
	}

	if(mp->dev) {
		mp->dev->mounted = false;
	}

	kfree(mp);
	return 0;
}

static size_t sfs_mounts_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	if(ctx->fp->offset) {
		return 0;
	}

	size_t rsize = 0;
	struct vfs_mountpoint* mp = mountpoints;
	for(; mp; mp = mp->next) {
		if(mp->dev) {
			sysfs_printf("/dev/%s %s %s rw,noatime 0 0\n", mp->dev->name, mp->path, mp->type);
		} else {
			sysfs_printf("%s %s %s rw,noatime 0 0\n", mp->type, mp->path, mp->type);
		}
	}
	return rsize;
}

void vfs_mount_init(const char* root_path) {
	if(vfs_mount(NULL, root_path, "/", 0) < 0) {
		panic("vfs: Could not mount root filesystem\n");
	}

	struct vfs_callbacks sfs_cb = {
		.read = sfs_mounts_read,
	};
	sysfs_add_file("mounts", &sfs_cb);
}
