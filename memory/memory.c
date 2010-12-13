#include <common/log.h>
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
	log("frames: Initialized\n");
	paging_init();
	log("paging: Initialized\n");
	log("memory: Initialized\n");
}
