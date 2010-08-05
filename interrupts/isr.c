#include <interrupts/isr.h>
#include <devices/display/interface.h>


// This gets called from our ASM interrupt handler stub.
void isr_handler(registers_t regs)
{
   print("received interrupt: ");
   display_printHex(regs.int_no);
   print("\n");

}
