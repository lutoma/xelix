#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

static const char* humanize(uint64_t bytes) {
	char *suffix[] = {"B", "KB", "MB", "GB", "TB"};
	char length = sizeof(suffix) / sizeof(suffix[0]);

	int i = 0;
	double dblBytes = bytes;

	if (bytes > 1024) {
		for (i = 0; (bytes / 1024) > 0 && i<length-1; i++, bytes /= 1024)
			dblBytes = bytes / 1024.0;
	}

	char* output;
	asprintf(&output, "%.02lf %s", dblBytes, suffix[i]);
	return output;
}


int main(int argc, char* argv[]) {
	FILE* fp = fopen("/sys/memfree", "r");
	if(!fp) {
		perror("Could not open /sys/memfree");
		exit(EXIT_FAILURE);
	}

	bool human = (argc > 1 && !strcmp(argv[1], "-h"));

	uint32_t total;
	uint32_t free;
	if(fscanf(fp, "%d %d\n", &total, &free) != 2) {
		fprintf(stderr, "Matching error.\n");
	}

	printf("%18s%13s%13s\n", "total", "used", "free");

	if(!human) {
		printf("Mem: %13d%13d%13d\n", total, total - free, free);
	} else {
		printf("Mem: %13s%13s%13s\n", humanize(total), humanize(total - free), humanize(free));
	}
	exit(EXIT_SUCCESS);
}
