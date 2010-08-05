#ifndef MEMORY_KHEAP_H
#define MEMORY_KHEAP_H

#include <common/generic.h>

uint32 kmalloc_int(uint32 sz, int align, uint32 *phys);
uint32 kmalloc_a(uint32 sz);  // page aligned.
uint32 kmalloc_p(uint32 sz, uint32 *phys); // returns a physical address.
uint32 kmalloc_ap(uint32 sz, uint32 *phys); // page aligned and returns a physical address.
uint32 kmalloc(uint32 sz); // vanilla (normal).
#endif
