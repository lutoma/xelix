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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <signal.h>
#include <limits.h>
#include <pwd.h>
#include <utmp.h>
#include <time.h>
#include "util.h"

void write_utmp(struct passwd* pwd) {
	struct utmp ut = {
		.ut_type = USER_PROCESS,
		.ut_pid = getpid(),
		.ut_id = "1",
		.ut_time = time(NULL),
		.ut_session = 1,
	};

	char* tname = ttyname(0);
	// Skip leading /dev/
	if(tname && strlen(tname) > 5) {
		strncpy(ut.ut_line, tname + 5, UT_LINESIZE);
	}

	strncpy(ut.ut_user, pwd->pw_name, UT_NAMESIZE);
	pututline(&ut);
}

int main(int argc, char* argv[], char* env[]) {
	sigset_t set;
	sigfillset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);

	char hostname[300];
	gethostname(hostname, 300);
	char* sname = shortname(strdup(hostname));

	char tty_buf[PATH_MAX];
	char* tty = "";
	if(ttyname_r(STDIN_FILENO, tty_buf, PATH_MAX) == 0) {
		tty = basename(tty_buf);
	}

	printf("\033[H\033[J");
	while(true) {
        printf("\nxelix %s\n\n%s ", tty, sname);
		struct passwd* pwd = do_auth(NULL);
		if(pwd) {
			write_utmp(pwd);
			run_shell(pwd, true);
		} else {
			fprintf(stderr, "Login incorrect.\n");
		}
	}

	free(sname);
}
