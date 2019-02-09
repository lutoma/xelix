#ifndef INCLUDE_PICO_XELIX
#define INCLUDE_PICO_XELIX
#include "pico_config.h"
#include "pico_device.h"

void pico_xelix_destroy(struct pico_device *null);
struct pico_device *pico_xelix_create(const char *name);

#endif

