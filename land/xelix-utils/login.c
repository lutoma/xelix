#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <pwd.h>
#include <sys/wait.h>
#include <sys/xelix.h>
#include "util.h"

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
		if(!read) {
			if(errno) {
				perror("fgets");
				return 1;
			}
			continue;
		}

		printf("Password: ");
		fflush(stdout);

		char* pass = malloc(500);
		errno = 0;
		read = fgets(pass, 500, stdin);
		if(!read) {
			if(errno) {
				perror("fgets");
				return 1;
			}
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

		FILE* motd_fp = fopen("/etc/motd", "r");
		if(motd_fp) {
			char* motd = (char*)malloc(1024);
			size_t read = fread(motd, 1024, 1, motd_fp);
			puts(motd);
			free(motd);
		}

		char* __argv[] = { pwd->pw_shell, "-l", NULL };

		char env_user[50];
		snprintf(env_user, 50, "USER=%s", pwd->pw_name);

		char env_home[100];
		snprintf(env_home, 100, "HOME=%s", pwd->pw_dir);

		char env_pwd[100];
		snprintf(env_pwd, 100, "PWD=%s", pwd->pw_dir);

		char env_host[306];
		snprintf(env_pwd, 306, "HOST=%s", sname);

		char* __env[] = { "PS1=[$USER@$HOST $PWD]# ", env_home, "TERM=vt100", env_pwd, env_user, env_host, NULL };

		chdir(pwd->pw_dir);

		int pid = fork();
		if(pid == -1) {
			perror("Could not fork");
			exit(-1);
		}

		if(!pid) {
			execve(pwd->pw_shell, __argv, __env);
			perror("Could not launch shell");
			exit(EXIT_FAILURE);
		}
		wait(NULL);
	}

	free(sname);
}
