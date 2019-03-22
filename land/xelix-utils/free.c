/* Copyright © 2018-2019 Lukas Martini
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
#include <stdbool.h>
#include <string.h>
#include "util.h"
#include "argparse.h"

static const char *const usage[] = {
    "free [options]",
    NULL,
};

int main(int argc, const char** argv) {
	int human;
	const char* path = "/sys/memfree";
	struct argparse_option options[] = {
		OPT_BOOLEAN(0, "help", NULL, "show this help message and exit", argparse_help_cb, 0, OPT_NONEG),
		OPT_BOOLEAN('h', "human", &human, "show human-readable output"),
		OPT_STRING('f', "file", &path, "file to read memory information from"),
        OPT_END(),
	};

    struct argparse argparse;
    argparse_init(&argparse, options, usage, 0);
    argparse_describe(&argparse, "\nDisplay amount of free and used memory in the system.",
    	"\nfree displays the total amount of free and used physical and swap "
    	"memory in the system, as well as the buffers and caches used by the "
    	"kernel. The information is gathered by parsing /sys/memfree.\nfree is "
    	"part of xelix-utils. Please report bugs to <hello@lutoma.org>.");
    argc = argparse_parse(&argparse, argc, argv);

	FILE* fp = fopen(path, "r");
	if(!fp) {
		perror("Could not read status");
		exit(EXIT_FAILURE);
	}

	uint32_t total;
	uint32_t free;
	if(fscanf(fp, "%d %d\n", &total, &free) != 2) {
		fprintf(stderr, "Matching error.\n");
	}

	printf("%18s%13s%13s\n", "total", "used", "free");

	if(!human) {
		printf("Mem: %13d%13d%13d\n", total, total - free, free);
	} else {
		printf("Mem: %13s%13s%13s\n", readable_fs(total), readable_fs(total - free), readable_fs(free));
	}
	exit(EXIT_SUCCESS);
}
