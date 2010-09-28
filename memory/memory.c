#include <memory/interface.h>
#include <memory/segmentation/gdt.h>
#include <memory/paging/frames.h>
#include <memory/paging/paging.h>
#include <common/generic.h>

void memory_init_preprotected()
{
	gdt_init();
	log("Initialized preprotected memory\n");
}

void memory_init_postprotected()
{
	frames_init();
	log("Initialized frames.\n");
	paging_init();
	log("Initialized paging.\n");
	log("Initialized postprotected memory\n");
}
