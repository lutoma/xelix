#include <interrupts/isr.h>
#include <devices/display/interface.h>


interruptHandler_t interrupt_handlers[256];

// This gets called from our ASM interrupt handler stub.
void isr_handler(registers_t regs)
{
	if (interrupt_handlers[regs.int_no] == 0)
	{
		log("received interrupt: ");
		display_printHex(regs.int_no);
		log("\n");
	}
	else
	{
		interruptHandler_t handler = interrupt_handlers[regs.int_no];
		handler(regs);
	}
}

void isr_registerHandler(uint8 n, interruptHandler_t handler)
{
	log("Registered ISR handler for ");
	display_printDec(n);
	log("\n");
	interrupt_handlers[n] = handler;
}
