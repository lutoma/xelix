#include <stdbool.h>
#include <sys/wait.h>
#include <sys/xelix.h>
#include "util.h"

int main() {
	char* net_argv[] = { "networkd", NULL };
	char* net_env[] = { "USER=root", NULL };
	execnew("/usr/bin/networkd", net_argv, net_env);

	char* login_argv[] = { "login", NULL };
	char* login_env[] = { "USER=root", NULL };
	execnew("/usr/bin/login", login_argv, login_env);

	while(true) {
		wait(NULL);
	}
}
