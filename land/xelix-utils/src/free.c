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
	const char* path = "/sys/mem_info";
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
    	"kernel. The information is gathered by parsing /sys/mem_info.\nfree is "
    	"part of xelix-utils. Please report bugs to <hello@lutoma.org>.");
    argc = argparse_parse(&argparse, argc, argv);

	FILE* fp = fopen(path, "r");
	if(!fp) {
		perror("Could not read status");
		exit(EXIT_FAILURE);
	}

	uint32_t mem_total = 0, mem_used = 0;
	uint32_t mem_shared = 0, mem_cache = 0;
	uint32_t palloc_total = 0, palloc_used = 0;
	uint32_t kmalloc_total = 0, kmalloc_used = 0;

	while(!feof(fp)) {
		char name[100];
		uint32_t value;

		if(fscanf(fp, "%100[^:]: %u\n", &name, &value) != 2) {
			fprintf(stderr, "Matching error.\n");
			break;
		}

		if(!strcmp(name, "mem_total")) {
			mem_total = value;
		} else if(!strcmp(name, "mem_used")) {
			mem_used = value;
		} else if(!strcmp(name, "mem_shared")) {
			mem_shared = value;
		} else if(!strcmp(name, "mem_cache")) {
			mem_cache = value;
		} else if(!strcmp(name, "palloc_total")) {
			palloc_total = value;
		} else if(!strcmp(name, "palloc_used")) {
			palloc_used = value;
		} else if(!strcmp(name, "kmalloc_total")) {
			kmalloc_total = value;
		} else if(!strcmp(name, "kmalloc_used")) {
			kmalloc_used = value;
		}
	}

	uint32_t mem_free = mem_total - mem_used;
	uint32_t palloc_free = palloc_total - palloc_used;
	uint32_t kmalloc_free = kmalloc_total - kmalloc_used;

	printf("%25s%13s%13s%13s%13s%13s\n", "total", "used", "free", "shared", "buff/cache", "available");

	if(!human) {
		printf("Mem:        %13u%13u%13u%13u%13u%13u\n",
			mem_total / 1024, mem_used / 1024, mem_free / 1024,
			mem_shared / 1024, mem_cache / 1024, mem_free / 1024);

		printf("palloc:     %13u%13u%13u\n", palloc_total / 1024,
			palloc_used / 1024, palloc_free / 1024);

		printf("kmalloc:    %13u%13u%13u\n", kmalloc_total / 1024,
			kmalloc_used / 1024, kmalloc_free / 1024);
	} else {
		printf("Mem:        %13s%13s%13s%13s%13s%13s\n", readable_fs(mem_total),
			readable_fs(mem_used), readable_fs(mem_free),
			readable_fs(mem_shared), readable_fs(mem_cache), readable_fs(mem_free));

		printf("palloc:     %13s%13s%13s\n", readable_fs(palloc_total),
			readable_fs(palloc_used), readable_fs(palloc_free));

		printf("kmalloc:    %13s%13s%13s\n", readable_fs(kmalloc_total),
			readable_fs(kmalloc_used), readable_fs(kmalloc_free));
	}
	exit(EXIT_SUCCESS);
}
