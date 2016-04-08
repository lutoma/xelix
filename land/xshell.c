#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>

// Xelix special interface™
#include <sys/execnew.h>

int main(int argc, char* argv[]) {
	if(argc > 1 && !strcmp(argv[1], "--hello")) {
		printf("Hello world.\n");
		return 0;
	}

	char* cwd = malloc(PATH_MAX);

	while(true) {
		getcwd(cwd, PATH_MAX);

		printf("xelix %s# ", cwd);

		// Will only print after \n otherwise
		fflush(stdout);

		char* cmd = malloc(500);
		char* read = fgets(cmd, 500, stdin);

		if(!read) {
			return 1;
		}

		// Strip the \n
		cmd[strlen(cmd) - 1] = (char)0;

		if(strlen(cmd) < 1) {
			continue;
		}

		if(cmd[0] == '#') {
			continue;
		}

		if(!strcmp(cmd, "exit")) {
			exit(EXIT_SUCCESS);
		}

		int echo_len = strlen("echo");
		if(!strncmp(cmd, "echo", echo_len)) {
			int offset = echo_len;

			if(strlen(cmd) > echo_len + 1) {
				offset++;
			}

			printf("%s\n", cmd + offset);
			continue;
		}

		char* __env[] = { "PS1=[$USER@$HOST $PWD]# ", "HOME=/root", "TERM=dash", "PWD=/", "USER=root", "HOST=default", NULL }; 
		char* __argv[] = { cmd, NULL };

		pid_t pid = execnew(cmd, __argv, __env);

		if(pid > 0) {
			// Wait for child to exit
			pid_t pr = wait(NULL);
		} else {
			printf("xshell: command not found: %s\n", cmd);
		}
	}
}
