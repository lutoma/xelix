#ifndef PICO_SUPPORT_XELIX
#define PICO_SUPPORT_XELIX

#include <time.h>
#include <log.h>
#include <mem/kmalloc.h>

// #define dbg(args...) log(LOG_INFO, "picotcp: " args);
#define dbg(...)

#define stack_fill_pattern(...) do {} while(0)
#define stack_count_free_words(...) do {} while(0)
#define stack_get_free_words() (0)

#define pico_zalloc(x) zmalloc(x)
#define pico_free(x) kfree(x)

static inline uint32_t PICO_TIME(void) {
	return time_get();
}

static inline uint64_t PICO_TIME_MS(void) {
	return (uint64_t)(time_get() * 1000);
}

static inline void PICO_IDLE(void) {
	//usleep(5000);
}

#endif  /* PICO_SUPPORT_XELIX */

