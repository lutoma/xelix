#include <stdio.h>
#include <stdlib.h>

#define fail(x) perror(x); return -1;

int main() {
	FILE* fp = fopen("/sys/log", "r");
	if(!fp) {
		fail("Could not open log file");
	}

	char* buffer = malloc(501);
	if(!fp) {
		fail("Could not allocate buffer");
	}

	size_t read;
	while(read = fread(buffer, 1, 500, fp)) {
		buffer[read] = 0;
		printf(buffer);
	}

	if(read < 0) {
		fail("Could not read log");
	}

	fclose(fp);
	free(buffer);
	return 0;
}
