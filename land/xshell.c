#ifdef __linux__
  // Needed for execvpe header
  #define _GNU_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>

#ifdef __xelix__
  // Xelix special interface™
  #include <sys/execnew.h>
#endif

static inline bool run_command(char* cmd, char* _argv[], char* _env[]) {
	#ifdef __xelix__
		return execnew(cmd, _argv, _env);
	#else
		pid_t pid = fork();

		if(pid) {
			return true;
		}

		execvpe(cmd, _argv, _env);
		perror("Error");
		exit(EXIT_FAILURE);
		return false;
	#endif
}

int main(int argc, char* argv[]) {
	char* cwd = malloc(PATH_MAX);
	char* cmd = malloc(500);

	while(true) {
		getcwd(cwd, PATH_MAX);

		printf("xelix %s# ", cwd);

		// Will only print after \n otherwise
		fflush(stdout);

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

		char* _env[] = { "PS1=[$USER@$HOST $PWD]# ", "HOME=/root", "TERM=dash", "PWD=/", "USER=root", "HOST=default", NULL };
		char* _argv[] = { cmd, NULL };
		//char** _argv = parse_arguments(cmd);

		bool r = run_command(cmd, _argv, _env);

		if(r > 0) {
			// Wait for child to exit
			pid_t pr = wait(NULL);
		} else {
			printf("xshell: command not found: %s\n", cmd);
		}
	}
}
