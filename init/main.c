#include <common/multiboot.h>
#include <common/generic.h>
#include <devices/display/interface.h>
#include <common/acpi.h>
#include <devices/cpu/interface.h>
#include <devices/keyboard/interface.h>
#include <memory/segmentation/gdt.h>
#include <interrupts/idt.h>
#include <interrupts/irq.h>
#include <devices/pit/interface.h>
#include <init/debugconsole.h>
#include <memory/kmalloc.h>
#include <common/bitmap.h>
#include <filesystems/interface.h>

void checkIntLenghts();

void checkIntLenghts()
{
	log("Checking length of uint8... ");
	ASSERT(sizeof(uint8) == 1);
	log("Right\n");
	
	log("Checking length of uint16... ");
	ASSERT(sizeof(uint16) == 2);
	log("Right\n");
	
	log("Checking length of uint32... ");
	ASSERT(sizeof(uint32) == 4);
	log("Right\n");
}

void kmain(struct multiboot *mboot_ptr)
{
	log_init();
	display_init();
	
	display_setColor(0x0f);
	print("                                               \n");
	print("                                     decore    \n");
	print("                                               \n");
	display_setColor(0x07);
	
	
	
	log("Initialized Display.\n");
	initAcpi();
	log("Initialized ACPI (Advanced Configuration and Power Interface)\n");
	checkIntLenghts();
	cpu_init();
	log("Initialized CPU\n");
	gdt_init();
	log("Initialized GDT (global descriptor table)\n");
	idt_init();
	log("Initialized IDT (interrupt descriptor table)\n");
	pit_init(50); //50Hz
	log("Initialized PIT (programmable interrupt timer)\n");
	keyboard_init();
	log("Initialized keyboard\n");
	//memory_init();
	//log("Initialized memory.\n");
	
	//uint32 *ptr = (uint32*)0xA0000000;
	//uint32 do_page_fault = *ptr;
	

	log("Decore is up.\n");
	debugconsole_init();
	while(1)
	{
		
	}
}
