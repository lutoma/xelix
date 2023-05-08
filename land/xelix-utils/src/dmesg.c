/* Copyright © 2019-2023 Lukas Martini
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
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdbool.h>
#include "util.h"
#include "argparse.h"

struct log_entry {
	uint8_t level;
	uint64_t tick;
	uint32_t timestamp;
	uint32_t length;
};

static const char *const usage[] = {
    "dmesg [options]",
    NULL,
};

#define fail(x) perror(x); return -1;

int main(int argc, const char** argv) {
	int human;
	int show_ticks;
	int since;
	const char* path = "/sys/log";
	struct argparse_option options[] = {
		OPT_HELP(),
		OPT_STRING('f', "file", &path, "file to read kernel log from"),
        OPT_END(),
	};

    struct argparse argparse;
    argparse_init(&argparse, options, usage, 0);
    argparse_describe(&argparse, "print or control the kernel log buffer.",
    	"\ndmesg is used to examine or control the kernel log buffer. The "
    	"default action is to display all messages from the kernel log.\ndmesg "
    	"is part of xelix-utils. Please report bugs to <hello@lutoma.org>.");
    argc = argparse_parse(&argparse, argc, argv);

	int fd = open(path, O_RDONLY);
	if(fd == -1) {
		fail("Could not open log file");
	}

	while(true) {
		struct log_entry header;
		size_t hdr_len = sizeof(struct log_entry);

		size_t nread = read(fd, &header, hdr_len);
		if(nread != hdr_len) {
			// EOF
			if(nread == 0) {
				exit(EXIT_SUCCESS);
			}

			fail("Incomplete header entry");
		}

		char msg[header.length];
		if(read(fd, &msg, header.length) != header.length) {
			fail("Incomplete message");
		}

		char* level_name = "31m???";
		switch(header.level) {
			case 1: level_name = "39mDebug"; break;
			case 2: level_name = "33mInfo"; break;
			case 3: level_name = "35mWarn"; break;
			case 4: level_name = "31mErr"; break;
		}

		if(header.timestamp) {
			printf("%19s ", time2str(header.timestamp, "%Y-%m-%d %H:%M:%S"));
		} else {
			char* tstr;
			asprintf(&tstr, "+%d ticks", header.tick);
			printf("%19s ", tstr);
			free(tstr);
		}
		printf("\033[%-8s\033[m %s", level_name, msg);
	}

	close(fd);
	return 0;
}
