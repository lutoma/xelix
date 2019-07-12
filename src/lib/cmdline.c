/* cmdline.c: Kernel command line
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

#include <cmdline.h>
#include <string.h>
#include <mem/kmalloc.h>
#include <fs/sysfs.h>

static char* options[50] = {0};

char* cmdline_get(const char* key) {
	size_t key_len = strlen(key);

	for(int i = 0; i < ARRAY_SIZE(options) && options[i]; i++) {
		char* opt = options[i];

		if(strlen(opt) >= key_len + 2 && !strncmp(key, opt, key_len - 1)) {
			if(opt[key_len] != '=') {
				continue;
			}

			return &opt[key_len + 1];
		}
	}

	return NULL;
}

bool cmdline_get_bool(const char* key) {
	for(int i = 0; i < ARRAY_SIZE(options) && options[i]; i++) {
		if(!strcmp(options[i], key)) {
			return true;
		}
	}
	return false;
}

static size_t sfs_read(struct vfs_callback_ctx* ctx, void* dest, size_t size) {
	if(ctx->fp->offset) {
		return 0;
	}

	size_t rsize = 0;
	sysfs_printf("%s\n", cmdline_string());
	return rsize;
}

void cmdline_init() {
	char* cmdline = strdup(cmdline_string());
	size_t len = strlen(cmdline);

	char* cur = cmdline;
	char* prev = cmdline;
	int num_options = 0;
	for(int i = 0; i < len + 1; i++, cur++) {
		if(!*cur || *cur == ' ') {
			*cur = 0;
			options[num_options++] = prev;
			prev = cur + 1;
		}
	}

	struct vfs_callbacks sfs_cb = {
		.read = sfs_read,
	};
	sysfs_add_file("cmdline", &sfs_cb);
}
