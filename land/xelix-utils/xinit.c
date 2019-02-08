#include <stdbool.h>
#include <stdlib.h>
#include <signal.h>
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
	sigset_t set;
	sigfillset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);

	char* net_argv[] = { "networkd", NULL };
	char* net_env[] = { "USER=root", NULL };
	//launch("/usr/bin/networkd", net_argv, net_env);

	char* login_argv[] = { "login", NULL };
	char* login_env[] = { "USER=root", NULL };
	launch("/usr/bin/login", login_argv, login_env);

	while(true) {
		wait(NULL);
	}
}
