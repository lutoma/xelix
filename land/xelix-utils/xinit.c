#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <pwd.h>
#include <sys/wait.h>
#include <sys/xelix.h>

int main() {
	while(true) {
		printf("\nxelix tty1\n\n");
		printf("localhost login: ");
		fflush(stdout);

		char* user = malloc(50);
		char* read = fgets(user, 50, stdin);
		if(!read) {
			perror("fgets");
			return 1;
		}

		printf("Password: ");
		fflush(stdout);

		char* pass = malloc(500);
		read = fgets(pass, 500, stdin);
		if(!read) {
			perror("fgets");
			return 1;
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

		char* __argv[] = { pwd->pw_shell, "-il", NULL };
		char* __env[] = { "PS1=[$USER@$HOST $PWD]# ", "HOME=/root", "TERM=dash", "PWD=/", "USER=root", "HOST=default", NULL };

		pid_t shell = execnew(pwd->pw_shell, __argv, __env);
		if(shell < 1) {
			printf("xinit: Could not launch shell. Exiting.\n");
			exit(EXIT_FAILURE);
		}

		wait(NULL);
	}
}
