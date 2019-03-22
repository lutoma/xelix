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

#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include "util.h"

static void launch(const char* path, char** argv, char** env) {
	int pid = fork();
	if(pid == -1) {
		perror("Could not fork");
		exit(-1);
	}

	if(!pid) {
		execve(path, argv, env);
	}
}

int main() {
	if(getpid() != 1) {
		fprintf(stderr, "This program needs to be run as PID 1.\n");
		return -1;
	}

	sigset_t set;
	sigfillset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);

	char* login_argv[] = { "login", NULL };
	char* login_env[] = { "USER=root", NULL };

	while(true) {
		launch("/usr/bin/login", login_argv, login_env);
		wait(NULL);
	}
}
