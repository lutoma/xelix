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

// Set up environment in the forked task
static void setup_env(struct passwd* pwd) {
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

int main() {
	char hostname[300];
	gethostname(hostname, 300);
	char* sname = shortname(strdup(hostname));

	while(true) {
		printf("\nxelix tty1\n\n");
		printf("%s login: ", sname);
		fflush(stdout);

		char* user = malloc(50);
		errno = 0;
		char* read = fgets(user, 50, stdin);
		if(read < 0) {
			perror("fgets");
			continue;
		}

		printf("Password: ");
		fflush(stdout);

		struct termios termios;
		tcgetattr(0, &termios);
		termios.c_lflag &= ~ECHO;
		tcsetattr(0, TCSANOW, &termios);

		char* pass = malloc(500);
		errno = 0;
		read = fgets(pass, 500, stdin);

		termios.c_lflag |= ECHO;
		tcsetattr(0, TCSANOW, &termios);

		if(read < 0) {
			perror("fgets");
			continue;
		}

		// Strip the \n
		user[strlen(user) - 1] = (char)0;
		pass[strlen(pass) - 1] = (char)0;

		errno = 0;
		struct passwd* pwd = getpwnam(user);
		printf("\n");

		if(!pwd || strcmp(pass, pwd->pw_passwd)) {
			printf("Login incorrect.\n");
			continue;
		}

		int pid = fork();
		if(pid == -1) {
			perror("Could not fork");
			continue;
		}

		if(pid) {
			waitpid(pid, NULL, 0);
			printf("\033[H\033[J");
		} else {
			setup_env(pwd);
		}
	}

	free(sname);
}
