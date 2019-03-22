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
#include <errno.h>
#include <pwd.h>
#include <sys/wait.h>
#include <sys/termios.h>
#include "util.h"

static void run_shell(struct passwd* pwd) {
	int pid = fork();
	if(pid == -1) {
		perror("Could not fork");
		return;
	}

	if(pid) {
		waitpid(pid, NULL, 0);
		return;
	}

	if(pwd->pw_gid != getgid()) {
		if(setgid(pwd->pw_gid) < 0) {
			perror("Could not change group");
			exit(EXIT_FAILURE);
		}
	}
	if(pwd->pw_uid != getuid()) {
		if(setuid(pwd->pw_uid) < 0) {
			perror("Could not change user");
			exit(EXIT_FAILURE);
		}
	}

	chdir(pwd->pw_dir);

	printf("\033[H\033[J");
	FILE* motd_fp = fopen("/etc/motd", "r");
	if(motd_fp) {
		char* motd = (char*)malloc(1024);
		size_t read = fread(motd, 1024, 1, motd_fp);
		puts(motd);
		free(motd);
	}

	char* __argv[] = { pwd->pw_shell, "-l", NULL };
	char* __env[] = { "LOGIN=DONE", NULL };
	execve(pwd->pw_shell, __argv, __env);
	perror("Could not launch shell");
	exit(EXIT_FAILURE);
}

static struct passwd* do_auth(char* sname) {
	printf("\nxelix tty1\n\n");
	printf("%s login: ", sname);
	fflush(stdout);

	char user[50];
	char* read = fgets(user, 50, stdin);
	if(!read && !feof(stdin)) {
		perror("fgets");
		return NULL;
	}

	printf("Password: ");
	fflush(stdout);

	struct termios termios;
	tcgetattr(0, &termios);
	termios.c_lflag &= ~ECHO;
	tcsetattr(0, TCSANOW, &termios);

	char pass[500];
	read = fgets(pass, 500, stdin);

	termios.c_lflag |= ECHO;
	tcsetattr(0, TCSANOW, &termios);

	if(!read && !feof(stdin)) {
		perror("fgets");
		return NULL;
	}

	// Strip the \n
	user[strlen(user) - 1] = (char)0;
	pass[strlen(pass) - 1] = (char)0;

	struct passwd* pwd = getpwnam(user);
	printf("\n");

	if(!pwd || strcmp(pass, pwd->pw_passwd)) {
		return NULL;
	}

	return pwd;
}

int main() {
	sigset_t set;
	sigfillset(&set);
	sigdelset(&set, SIGCHLD);
	sigprocmask(SIG_SETMASK, &set, NULL);

	char hostname[300];
	gethostname(hostname, 300);
	char* sname = shortname(strdup(hostname));

	while(true) {
		struct passwd* pwd = do_auth(sname);
		if(pwd) {
			run_shell(pwd);
			printf("\033[H\033[J");
		} else {
			fprintf(stderr, "Login incorrect.\n");
		}
	}

	free(sname);
}
