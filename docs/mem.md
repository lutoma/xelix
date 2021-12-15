# Memory management

!!! note
	Xelix's memory management is currently undergoing heavy refactoring. Some of this documentation may be outdated by the time you read this.


## Page allocations

Memory pages can be allocated using

```c
// Allocate physical pages
void* mem = palloc(pages);

// Allocate virtual pages
void* mem = valloc(pages, context);

// Allocate physical and virtual page, and map them together
void* mem = pvalloc(pages, context);

pfree(num, pages);
vfree(num, pages);
pvfree(num, pages);
```

## Physical memory

The Xelix kernel is currently always located at 0x100000 in physical memory.

Physical memory is allocated using a page allocator (`mem/palloc.c`). It is the fastest method of memory allocation in Xelix, but has two significant limitations: It can only allocate full pages (4KB on x86), and it does not keep information on the size of allocations. Due to this, to free an allocation, the size needs to be supplied as well.

It is best suited for large, long-term allocations where the size is fixed or stored in a side channel, or for allocations that need to align to page boundaries anyway (like task memory).

```c
#include <mem/palloc.h>
void* palloc(uint32_t num_pages);
void pfree(uint32_t page_id, uint32_t num_pages);
```

## Kernel virtual memory space

Xelix used to have a strict 1:1 mapping of kernel virtual memory to physical memory. While this is simple and convenient for debugging, it had the significant disadvantage of not making Zero-copy mapping from userland tasks possible.

## kmalloc

kmalloc is the internal memory allocator of the kernel. It is used for small allocations (smaller than one page, generally) that will have to be freed again at some point. Memory allocated using kmalloc should never be used in task address space.

```c
#include <mem/kmalloc.h>

// Allocate memory
void* mem = kmalloc(size);

// Allocate memory and initialize it to zeros
void* mem = zmalloc(size);

// Allocate page-aligned memory -- If you need a whole page, you should use palloc instead
void* mem = kmalloc_a(size);

// Allocate page-aligned memory and initialize it to zeros
void* mem = zmalloc_a(size);

// Free allocated memory
kfree(mem);
```

kmalloc can optionally be compiled with checks for out-of-bounds writes/memory overflows. This works by placing canary values before and after each allocation and checking them on calls to `free()`. This option should only be enabled for debug builds due to the performance penalty it incurs.

## Userland-visible kernel memory: UL_VISIBLE()

The kernel binary and its memory regions cannot be read by userland tasks since they run in Ring 3 and don't have permission to access that Ring 0 memory. As an additional protection and to keep task memory from being cluttered, the kernel binary is not mapped into task/userland virtual memory at all.

This presents an issue during interrupts and context switches -- While the CPU switches us back to Ring 0 as soon as the interrupt handler is called, we will still be in the paging context of the task until we manually switch it. But the interrupt handler and more importantly the variable which contains the correct paging context to restore are in kernel memory and thus would be inacessible before the switch.

To solve this chicken-and-egg problem, Xelix has a macro called `UL_VISIBLE`, which can be used to mark variables and functions for mapping into userspace/task memory. This is accomplished by placing these into a different ELF section by the linker script.
