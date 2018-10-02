#ifndef PICO_SUPPORT_XELIX
#define PICO_SUPPORT_XELIX

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/time.h>

#define dbg printf

#define stack_fill_pattern(...) do {} while(0)
#define stack_count_free_words(...) do {} while(0)
#define stack_get_free_words() (0)

#define pico_zalloc(x) calloc(x, 1)
#define pico_free(x) free(x)

static inline uint32_t PICO_TIME(void) {
	struct timeval t;
	if(gettimeofday(&t, NULL) == -1) {
		perror("Could not get time");
		exit(EXIT_FAILURE);
	}
	return (uint32_t)t.tv_sec;
}

static inline uint64_t PICO_TIME_MS(void) {
	struct timeval t;
	uint64_t time;

	if(gettimeofday(&t, NULL) == -1) {
		perror("Could not get time");
		exit(EXIT_FAILURE);
	}

	time = (uint64_t)((t.tv_sec * 1000) + (t.tv_usec / 1000));
	return time;
}

static inline void PICO_IDLE(void) {
	printf("sleep stub.\n");
	//usleep(5000);
}

#endif  /* PICO_SUPPORT_XELIX */

