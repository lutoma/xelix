#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>

int main(int argc, char* argv[]) {
	char* dir;

	if(argc >= 2) {
		dir = argv[1];
	} else {
		dir = malloc(100);

		if(!getcwd(dir, 100)) {
			perror("getcwd failed");
			free(dir);
			exit(EXIT_FAILURE);
		}
	}

	DIR* dd = opendir(dir);
	free(dir);

	if(!dd) {
		perror("opendir failed");
		exit(EXIT_FAILURE);
	}

	struct dirent *ep;
	while (ep = readdir (dd)) {
		printf("%s ", ep->d_name);
	}

	printf("\n");

	//closedir(dd);
	return EXIT_SUCCESS;
}
