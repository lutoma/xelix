#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

int main() {
	char* dir = malloc(100);

	if(!getcwd(dir, 100)) {
		perror("getcwd failed");
		free(dir);
		exit(EXIT_FAILURE);
	}

	DIR* dd = opendir(dir);
	free(dir);

	if(!dd) {
		perror("opendir failed");
		exit(EXIT_FAILURE);
	}

	struct dirent *ep;
	while (ep = readdir (dd)) {
		puts(ep->d_name);
	}

	closedir(dd);
	return EXIT_SUCCESS;
}
