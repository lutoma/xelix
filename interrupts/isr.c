#include <interrupts/isr.h>


// This gets called from our ASM interrupt handler stub.
void isr_handler(registers_t regs)
{
   log("received interrupt: ");
   display_printHex(regs.int_no);
   log("\n");

}
