#include <stdio.h>
#include <stdlib.h>
#include <sys/xelix.h>

extern int main(int argc, char* argv);
extern char** environ;

void __attribute__((__fastcall__)) _start() {
	if(_xelix_getexecdata()) {
		fprintf(stderr, "_xelix_getexecdata() failed.\n");
		_exit(EXIT_FAILURE);
	}

	char** __argv = malloc(_xelix_execdata->argc * sizeof(char*));
	char** __env = malloc((400 + _xelix_execdata->envc) * sizeof(char*));

	uint32_t offset = 0;
	int i = 0;
	for(; i < _xelix_execdata->argc + _xelix_execdata->envc; i++) {
		char* arg = (char*)((intptr_t)_xelix_execdata->argv_environ + offset);

		if(i < _xelix_execdata->argc) {
			__argv[i] = arg;
		} else {
			__env[i - _xelix_execdata->argc] = arg;
		}

		offset += strlen(arg) + 1;
	}
	__env[i - _xelix_execdata->argc] = NULL;

	environ = __env;
	int ret = main(_xelix_execdata->argc, __argv);

	free(__env);
	free(__argv);
	_exit(ret);
}
