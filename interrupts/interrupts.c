#include <interrupts/interface.h>
#include <common/generic.h>
#include <interrupts/idt.h>




interruptHandler_t interruptHandlers[256];


void interrupts_init()
{
	idt_init();

	// set all interruptHandlers to zero
	memset(interruptHandlers, 0, 256*sizeof(interruptHandler_t));
	log("Initialized interrupts\n");
}

void interrupt_callback(registers_t regs)
{
	static int ininterrupt = 0;
	
	
	if(ininterrupt)
	{
		// double fault!!!
		PANIC("double fault!!!\n");
	}
	
	ininterrupt = 1;
	
	if (interruptHandlers[regs.int_no] == 0)
	{
		/*
		print("received interrupt: ");
		printHex(regs.int_no);
		print("\n");
		*/
	}
	else
	{
		interruptHandler_t handler = interruptHandlers[regs.int_no];
		handler(regs);
	}
	
	ininterrupt = 0;

}

void interrupt_registerHandler(uint8 n, interruptHandler_t handler)
{
	log("Registered IRQ handler for ");
	logDec(n);
	log(".\n");
	interruptHandlers[n] = handler;
}

