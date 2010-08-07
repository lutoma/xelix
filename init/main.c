#include <common/generic.h>
#include <devices/display/interface.h>
#include <devices/cpu/interface.h>
#include <devices/keyboard/interface.h>
#include <memory/interface.h>
#include <interrupts/idt.h>
#include <interrupts/irq.h>
#include <devices/pit/interface.h>
#include <memory/kmalloc.h>


#include <memory/paging/frames.h>

void checkIntLenghts();

void checkIntLenghts()
{
	log("Checking length of uint8... ");
	if(sizeof(uint8) == 1) log("Right\n");
	else panic("Got wrong lenght for uint8");
	
	log("Checking length of uint16... ");
	if(sizeof(uint16) == 2) log("Right\n");
	else panic("Got wrong lenght for uint16");
	
	log("Checking length of uint32... ");
	if(sizeof(uint32) == 4) log("Right\n");
	else panic("Got wrong lenght for uint32");
}

void kmain()
{
	
	display_init();
	
	display_setColor(0x0f);
	print("                                               \n");
	print("                                     decore    \n");
	print("                                               \n");
	display_setColor(0x07);
	
	
	
	log("Initialized Display.\n");
	checkIntLenghts();
	memory_init_preprotected();
	log("Initialized preprotected memory\n");
	cpu_init();
	log("Initialized CPU\n");
	idt_init();
	log("Initialized IDT (interrupt descriptor table)\n");
	memory_init_postprotected();
	log("Initialized postprotected memory\n");
	pit_init(50); //50Hz
	log("Initialized PIT (programmable interrupt timer)\n");
	keyboard_init();
	log("Initialized keyboard\n");
	
	log("\nDecore is up.\n");
	
	
	
	
	
	while(1)
	{
		
	}
}
