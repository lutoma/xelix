#include <memory/interface.h>
#include <memory/segmentation/gdt.h>
#include <memory/paging/frames.h>
#include <common/generic.h>

void memory_init_preprotected()
{
	gdt_init();
	log(" Initialised GDT.\n");
}

void memory_init_postprotected()
{
	frames_init();
}
