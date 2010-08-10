#include <interrupts/interface.h>


// This gets called from our ASM interrupt handler stub.
void isr_handler(registers_t regs)
{
	interrupt_callback(regs);
}
