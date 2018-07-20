#include <stdio.h>

extern int main(int argc, char* argv);
extern char **environ;

void _start() {
	char** __env = malloc(1024 * 400);
	bzero(__env, 1024 * 400);
	*environ = __env;

	char* __argv[] = { "init", NULL };
	int ret = main(1, __argv);

	free(__env);
	_exit(ret);
}
