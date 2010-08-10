#include <interrupts/interface.h>
#include <common/generic.h>
#include <interrupts/idt.h>




interruptHandler_t interruptHandlers[256];


void interrupts_init()
{
	idt_init();

	// set all interruptHandlers to zero
	memset(interruptHandlers, 0, 256*sizeof(interruptHandler_t));
}

void interrupt_callback(registers_t regs){
	if (interruptHandlers[regs.int_no] == 0)
	{
		/*
		log("received interrupt: ");
		display_printHex(regs.int_no);
		log("\n");
		*/
	}
	else
	{
		interruptHandler_t handler = interruptHandlers[regs.int_no];
		handler(regs);
	}

}

void interrupt_registerHandler(uint8 n, interruptHandler_t handler)
{
	log("Registered IRQ handler for ");
	logDec(n);
	log(".\n");
	interruptHandlers[n] = handler;
}

