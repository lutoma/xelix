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

	char buffer[80];
	strftime(buffer, 80, "%H:%M:%S", timeinfo);

	printf(" %s up %02d:%02d\n", buffer, uptime / 60, uptime % 60);

	fclose(fp);
	exit(EXIT_SUCCESS);
}
