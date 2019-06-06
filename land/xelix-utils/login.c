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
#include <pwd.h>
#include "util.h"

int main(int argc, char* argv[], char* env[]) {
	sigset_t set;
	sigfillset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);

	char hostname[300];
	gethostname(hostname, 300);
	char* sname = shortname(strdup(hostname));

	char* tty = ttyname(STDIN_FILENO);
	if(tty) {
		tty = basename(tty);
	}

	printf("\033[H\033[J");
	while(true) {
        printf("\nxelix %s\n\n%s ", tty, sname);
		struct passwd* pwd = do_auth(NULL);
		if(pwd) {
			run_shell(pwd, true);
		} else {
			fprintf(stderr, "Login incorrect.\n");
		}
	}

	free(sname);
}
