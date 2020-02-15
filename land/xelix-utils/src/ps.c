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
#include <stdbool.h>
#include <string.h>
#include <pwd.h>
#include "util.h"

int main(int argc, char* argv[]) {
	FILE* fp = fopen("/sys/tasks", "r");
	if(!fp) {
		perror("Could not read tasks");
		exit(EXIT_FAILURE);
	}

	// Drop first line
	char* data = malloc(1024);
	fgets(data, 1024, fp);
	free(data);

	printf("  PID User     State     PPID TTY      Mem\n");

	while(true) {
		if(feof(fp)) {
			return EXIT_SUCCESS;
		}

		uint32_t pid;
		uint32_t uid;
		uint32_t gid;
		uint32_t ppid;
		char cstate;
		char name[500];
		uint32_t mem;
		char _tty[30];
		char* tty = _tty;

		if(fscanf(fp, "%d %d %d %d %c \"%500[^\"]\" %d %s\n", &pid, &uid, &gid, &ppid, &cstate, name, &mem, &_tty) != 8) {
			fprintf(stderr, "Matching error.\n");
			exit(EXIT_FAILURE);
		}

		char* state = "Unknown";
		switch(cstate) {
			case 'K': state = "31mKilled"; break;
			case 'T': state = "31mTerminated"; break;
			case 'B': state = "35mBlocking"; break;
			case 'S': state = "36mStopped"; break;
			case 'R': state = "32mRunning"; break;
			case 'W': state = "39mWaiting"; break;
			case 'C': state = "33mSyscall"; break;
		}

		char* user;
		struct passwd* pwd = getpwuid(uid);
		if(pwd) {
			user = pwd->pw_name;
		} else {
			char _user[10];
			itoa(uid, _user, 10);
			user = _user;
		}

		char* rfs = readable_fs(mem);
		if(*tty == '/') {
			tty = basename(tty);
		}

		printf("%5d %-8s \033[%-11s\033[m %5d %-8s %-10s %-15s\n", pid, user, state, ppid, tty, rfs, name);
		free(rfs);
	}

	exit(EXIT_SUCCESS);
}
