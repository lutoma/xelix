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
#include <stdint.h>
#include <string.h>
#include <time.h>
#include "argparse.h"

static const char *const usage[] = {
    "uptime [options]",
    NULL,
};

int main(int argc, const char** argv) {
	int human;
	int show_ticks;
	int since;
	const char* path = "/sys/tick";
	struct argparse_option options[] = {
		OPT_HELP(),
		OPT_BOOLEAN('t', "ticks", &show_ticks, "show number of elapsed interrupt ticks"),
		OPT_BOOLEAN('s', "since", &since, "system up since"),
		OPT_STRING('f', "file", &path, "file to read memory information from"),
        OPT_END(),
	};

    struct argparse argparse;
    argparse_init(&argparse, options, usage, 0);
    argparse_describe(&argparse, "Tell how long the system has been running.",
    	"\nuptime gives a one line display of the "
    	"following information. The current time, how long the system has "
    	"been running, how many users are currently logged on, and the system "
    	"load averages for the past 1, 5, and 15 minutes.\nuptime is part of "
    	"xelix-utils. Please report bugs to <hello@lutoma.org>.");
    argc = argparse_parse(&argparse, argc, argv);

	FILE* fp = fopen(path, "r");
	if(!fp) {
		perror("Could not read tick");
		exit(EXIT_FAILURE);
	}

	uint32_t uptime;
	uint32_t ticks;
	uint32_t tick_rate;

	if(fscanf(fp, "%d %d %d\n", &uptime, &ticks, &tick_rate) != 3) {
		fprintf(stderr, "Matching error.\n");
	}

	time_t rtime = time(NULL);

	if(since) {
		puts(time2str(rtime - uptime, "%Y-%m-%d %H:%M:%S"));
		exit(EXIT_SUCCESS);
	}

	int hours = uptime / (60*60);
	int minutes = uptime % (60*60) / 60;
	char* time_str = time2str(rtime, "%H:%M:%S");
	printf(" %s up ", time_str);

	if(hours) {
		printf("%d:%02d\n", hours, minutes);
	} else {
		printf("%d min\n", minutes);
	}

	if(show_ticks) {
		printf(" %d PIT ticks, %d Hz tick rate\n", ticks, tick_rate);
	}

	fclose(fp);
	exit(EXIT_SUCCESS);
}
