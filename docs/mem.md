# Memory management

## Address space layout

Kernel address space is currently always in a flat 1:1 mapping. The kernel is always loaded at 0x100000 in physical memory.

## Physical memory

Physical memory is allocated using a page allocator (`mem/palloc.c`). It is the fastest method of memory allocation in Xelix, but has two significant limitations: It can only allocate full pages (4KB on x86), and it does not keep information on the size of allocations. Due to this, to free an allocation, the size needs to be supplied as well.

It is best suited for large, long-term allocations where the size is fixed or stored in a side channel, or for allocations that need to align to page boundaries anyway (like task memory).

```c
#include <mem/palloc.h>
void* palloc(uint32_t num_pages);
void pfree(uint32_t page_id, uint32_t num_pages);
```

## kmalloc

kmalloc is a memory allocator that is used exclusively for internal memory allocations.
