/* Copyright © 2019 Lukas Martini
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

#include <unistd.h>
#include <stdio.h>
#include "util.h"

char* shortname(char* in) {
	for(char* i = in; *i; i++) {
		if(*i == '.') {
			*i = 0;
			break;
		}
	}
	return in;
}

char* readable_fs(uint64_t bytes) {
    char *suffix[] = {"B", "KB", "MB", "GB", "TB"};
    char length = sizeof(suffix) / sizeof(suffix[0]);

    int i = 0;
    double dblBytes = bytes;

    if (bytes > 1024) {
        for (i = 0; (bytes / 1024) > 0 && i<length-1; i++, bytes /= 1024)
            dblBytes = bytes / 1024.0;
    }

    char* output;
    asprintf(&output, "%.02lf %s", dblBytes, suffix[i]);
    return output;
}

static char time_str[200];
char* time2str(time_t rtime, char* fmt) {
    struct tm* timeinfo = localtime(&rtime);
    strftime(time_str, 200, fmt, timeinfo);
    return time_str;
}
