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
#include <sys/mount.h>
#include <mntent.h>
#include "util.h"

int main(int argc, const char** argv) {
	if(argc == 1) {
		FILE* mntfp = setmntent(_PATH_MOUNTED, "r");
		if(!mntfp) {
			fprintf(stderr, "Could not open %s: %s\n", _PATH_MOUNTED, strerror(errno));
			return 1;
		}

		struct mntent* ent;
		while(NULL != (ent = getmntent(mntfp))) {
			printf("%s on %s type %s (%s)\n", ent->mnt_fsname, ent->mnt_dir,
				ent->mnt_type, ent->mnt_opts);
		}

		endmntent(mntfp);
		return 0;
	} else if(argc == 3) {
		if(mount(argv[1], argv[2], "ext2", 0, NULL) < 0) {
			fprintf(stderr, "Could not mount %s to %s: %s\n", argv[1], argv[2], strerror(errno));
			return 1;
		}
		return 0;
	}

	fprintf(stderr, "Usage:\n"
		"    mount\n"
		"    mount <source> <directory>\n\n"
		"view mount points and mount file systems\n\n"
		"Options:\n"
		"    -h, --help     display this help\n\n"
    	"mount is part of xelix-utils. "
    	"Please report bugs to <hello@lutoma.org>.\n");
	return 1;
}
