#include <interrupts/interface.h>
#include <devices/display/interface.h>

// This gets called from our ASM irq handler stub.
void irq_handler(registers_t regs)
{
	// Send an EOI (end of interrupt) signal to the PICs.
	// If this interrupt involved the slave.
	if (regs.int_no >= 40)
	{
		// Send reset signal to slave.
		outb(0xA0, 0x20);
	}
	
	//log("Sent EOI\n");
	// Send reset signal to master. (As well as slave, if necessary).
	outb(0x20, 0x20);
	
	
	interrupt_callback(regs);
}

