/* Copyright © 2020 Lukas Martini
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
 * along with Xelix. If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <limits.h>
#include <mntent.h>
#include <sys/mount.h>
#include "util.h"

FILE* mntfp = NULL;
void cleanup() {
	if(mntfp) {
		endmntent(mntfp);
	}
}

int main(int argc, const char** argv) {
	atexit(cleanup);

	if(argc == 2) {
        char* path = realpath(argv[1], NULL);
        if(!path) {
        	fprintf(stderr, "umount: %s: Could not resolve path: %s\n", argv[1], strerror(errno));
        	return 1;
        }

		mntfp = setmntent(_PATH_MOUNTED, "r");
		if(!mntfp) {
			fprintf(stderr, "umount: Could not open %s: %s\n", _PATH_MOUNTED, strerror(errno));
			return 1;
		}

		struct mntent* ent;
		while(NULL != (ent = getmntent(mntfp))) {
			if(!strcmp(ent->mnt_fsname, path) || !strcmp(ent->mnt_dir, path)) {
				if(umount2(ent->mnt_dir, 0) < 0) {
					fprintf(stderr, "umount: %s: umount failed: %s\n", path, strerror(errno));
					return 1;
				}

				return 0;
			}
		}

		fprintf(stderr, "umount: %s: Not mounted.\n", path);
		return 1;
	}

	fprintf(stderr, "Usage:\n"
		"    umount <source> | <directory>\n\n"
		"Unmount filesystems.\n\n"
		"Options:\n"
		"    -h, --help     display this help\n\n"
    	"umount is part of xelix-utils. "
    	"Please report bugs to <hello@lutoma.org>.\n");
	return 1;
}
