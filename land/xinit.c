#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <sys/execnew.h>

int main() {
	printf("\nxelix tty1\n\n");
	printf("localhost login: ");
	fflush(stdout);

	char* user = malloc(50);
	char* read = fgets(user, 50, stdin);
	if(!read) {
		return 1;
	}

	printf("Password: ");
	fflush(stdout);

	char* pass = malloc(500);
	read = fgets(pass, 500, stdin);
	if(!read) {
		return 1;
	}

	// Strip the \n
	user[strlen(user) - 1] = (char)0;
	pass[strlen(pass) - 1] = (char)0;

	printf("\n");

	char* __argv[] = { "xshell", "-l", NULL };
	char* __env[] = { "PS1=[$USER@$HOST $PWD]# ", "HOME=/root", "TERM=dash", "PWD=/", "USER=root", "HOST=default", NULL }; 

	while(true) {
		pid_t shell = execnew("/xshell", __argv, __env);
		wait(NULL);
		printf("xinit: Shell seems to have died, respawning it.\n");
	}
}
