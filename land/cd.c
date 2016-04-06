#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
	if(argc < 2) {
		fprintf(stderr, "Usage: cd <directory>\n");
		exit(EXIT_FAILURE);
	}

	chdir(argv[1]);
	exit(EXIT_SUCCESS);
}
