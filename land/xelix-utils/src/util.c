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
#include <pwd.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <errno.h>
#include <sys/termios.h>
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

char* readable_fs(uint64_t size) {
	char *suffix[] = {"B", "KB", "MB", "GB", "TB"};
	char length = sizeof(suffix) / sizeof(suffix[0]);

	int i = 0;
	double calc_size = size;
	if(size > 1024) {
		for(i = 0; (size / 1024) > 0 && i<length-1; i++, size /= 1024)
			calc_size = size / 1024.0;
	}

	char* output;
	asprintf(&output, "%.2lf %s", calc_size, suffix[i]);
	return output;
}

static char time_str[200];
char* time2str(time_t rtime, char* fmt) {
	struct tm* timeinfo = localtime(&rtime);
	strftime(time_str, 200, fmt, timeinfo);
	return time_str;
}


struct passwd* do_auth(char* user) {
	if(!user) {
		printf("login: ");
		fflush(stdout);

		char _user[50];
		errno = 0;
		if(!fgets(_user, 50, stdin)) {
			if(!feof(stdin) || errno) {
				perror("fgets");
			}
			return NULL;
		}
		user = _user;

		// Strip the \n
		*strchrnul(user, '\n') = '\0';

	}

	printf("Password: ");
	fflush(stdout);

	struct termios termios;
	tcgetattr(0, &termios);
	termios.c_lflag &= ~ECHO;
	tcsetattr(0, TCSANOW, &termios);

	char pass[500];
	errno = 0;
	char* res = fgets(pass, 500, stdin);
	int fgets_errno = errno;

	termios.c_lflag |= ECHO;
	tcsetattr(0, TCSANOW, &termios);

	if(!res) {
		if(!feof(stdin) || fgets_errno) {
			perror("fgets");
		}
		return NULL;
	}
	*strchrnul(pass, '\n') = '\0';

	struct passwd* pwd = getpwnam(user);
	printf("\n");

	if(!pwd || strcmp(pass, pwd->pw_passwd)) {
		return NULL;
	}

	return pwd;
}

void run_shell(struct passwd* pwd, bool print_motd) {
	if(pwd->pw_gid != getgid()) {
		if(setgid(pwd->pw_gid) < 0) {
			perror("Could not change group");
			return;
		}
	}
	if(pwd->pw_uid != getuid()) {
		if(setuid(pwd->pw_uid) < 0) {
			perror("Could not change user");
			return;
		}
	}

	chdir(pwd->pw_dir);
	if(print_motd) {
		printf("\033[H\033[J");
		FILE* motd_fp = fopen("/etc/motd", "r");
		if(motd_fp) {
			char* motd = (char*)malloc(1024);
			size_t read = fread(motd, 1024, 1, motd_fp);
			puts(motd);
			fflush(stdout);
			free(motd);
		}
	}

	char* __argv[] = { pwd->pw_shell, "-l", NULL };
	execv(pwd->pw_shell, __argv);
	perror("Could not launch shell");
	return;
}
