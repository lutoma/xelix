#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/limits.h>

#ifdef __xelix__
  // Xelix special interface™
  #include <sys/xelix.h>
#endif

static char** parse_arguments(char* args) {
	char* sp;
	int argc = 0;
	char* args_cpy = strdup(args);

	for(char* pch = strtok_r(args_cpy, " ", &sp); pch != NULL; pch = strtok_r(NULL, " ", &sp), argc++);
	free(args_cpy);

	char** _argv = malloc(sizeof(char*) * (argc + 1));
	sp = NULL;

	char* pch = strtok_r(args, " ", &sp);
	int i = 0;
	for(; pch != NULL; pch = strtok_r(NULL, " ", &sp), i++) {
		_argv[i] = pch;
	}

	_argv[i] = NULL;
	return _argv;
}

static inline bool run_command(char* cmd, char* _argv[], char* _env[]) {
	#ifdef __xelix__
		char* full_path = malloc(500);
		snprintf(full_path, 500, "/usr/bin/%s", cmd);
		return execnew(full_path, _argv, _env);
		free(full_path);
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
	char* user = getenv("USER");
	char* host = getenv("HOST");
	getcwd(cwd, PATH_MAX);

	while(true) {
		printf("%s@%s %s# ", user, host, cwd);

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

		int cd_len = strlen("cd");
		if(!strncmp(cmd, "cd", cd_len)) {
			int offset = cd_len;

			if(strlen(cmd) > cd_len + 1) {
				offset++;
			}

			if(chdir(cmd + offset) == -1) {
				perror("Could not change directory");
			} else {
				getcwd(cwd, PATH_MAX);
			}
			continue;
		}

		if(!strcmp(cmd, "pwd")) {
			printf("%s\n", cwd);
			continue;
		}

		char* _env[] = { "PS1=[$USER@$HOST $PWD]# ", "HOME=/root", "TERM=xterm-256color", "PWD=/", "USER=root", "HOST=default", NULL };
		char** _argv = parse_arguments(cmd);

		bool r = run_command(cmd, _argv, _env);
		free(_argv);

		if(r > 0) {
			// Wait for child to exit
			pid_t pr = wait(NULL);
		} else {
			printf("xshell: command not found: %s\n", cmd);
		}
	}
}
