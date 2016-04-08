#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/wait.h>

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

		char* __env[] = { "PS1=[$USER@$HOST $PWD]# ", "HOME=/root", "TERM=dash", "PWD=/", "USER=root", "HOST=default", NULL }; 
		char* __argv[] = { cmd, NULL };

		// Make the syscall
		asm volatile (
			"movl $0x1c, %%eax;\n"
			"movl %0, %%ebx;\n"
			"movl %1, %%ecx;\n"
			"movl %2, %%edx;\n"
			"int $0x80;\n"
			: /* No outputs */
			: "g" (cmd), "g" (__argv), "g" (__env)
			: "memory", "eax", "ebx", "ecx", "edx"
		);

		// Wait for child to exit
		pid_t pr = wait(NULL);
	}
}
