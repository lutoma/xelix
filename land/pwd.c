#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

int main() {
	char* cwd = malloc(PATH_MAX);
	getcwd(cwd, PATH_MAX);
	printf("%s\n", cwd);
	free(cwd);
	return EXIT_SUCCESS;
}
