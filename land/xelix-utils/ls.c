#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

int main(int argc, char* argv[]) {
	char* dir;

	if(argc >= 2) {
		dir = argv[1];
	} else {
		dir = ".";
	}

	DIR* dd = opendir(dir);

	if(!dd) {
		perror("opendir failed");
		exit(EXIT_FAILURE);
	}

	struct dirent *ep;
	while (ep = readdir (dd)) {
		if(!strcmp(ep->d_name, ".") || !strcmp(ep->d_name, "..")) {
			continue;
		}

		printf("%s ", ep->d_name);
	}

	printf("\n");

	//closedir(dd);
	return EXIT_SUCCESS;
}
