#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>

int main(int argc, char* argv[]) {
	FILE* fp = fopen("/sys/tick", "r");
	if(!fp) {
		perror("Could not read tick");
		exit(EXIT_FAILURE);
	}

	uint32_t uptime;
	uint32_t ticks;
	uint32_t tick_rate;

	if(fscanf(fp, "%d %d %d\n", &uptime, &ticks, &tick_rate) != 3) {
		fprintf(stderr, "Matching error.\n");
	}

	time_t rtime = time(NULL);
	struct tm* timeinfo = localtime(&rtime);
	char* ftime = asctime(timeinfo);
	ftime[strlen(ftime) - 1] = 0;

	printf("%s up %d seconds\n", ftime, uptime);
	exit(EXIT_SUCCESS);
}
