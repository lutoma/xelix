#include <stdio.h>
#include <stdlib.h>

int main(int argc, char* argv[]) {
	if(argc < 2) {
		fprintf(stderr, "Usage: %s <file>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	char* file = argv[1];

	FILE* fp = fopen(file, "r");
	if(!fp) {
		perror("fopen");
		exit(EXIT_FAILURE);
	}

	char* data = malloc(1024);
	size_t read = fread(data, 1024, 1, fp);

	puts(data);
	exit(EXIT_SUCCESS);
}
