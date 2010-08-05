#include <interrupts/irq.h>
#include <devices/display/interface.h>

// TODO multiple interrupt handlers per Interrupt?
irqHandler_t interrupt_handlers[256];



// This gets called from our ASM interrupt handler stub.
void irq_handler(registers_t regs)
{
   //log("Received IRQ.\n");
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
  
   if (interrupt_handlers[regs.int_no] != 0)
   {
       irqHandler_t handler = interrupt_handlers[regs.int_no];
       handler(regs);
   }
   
}

void irq_registerHandler(uint8 n, irqHandler_t handler)
{
  log("Registered IRQ handler for ");
  display_printDec(n);
  log("\n");
  interrupt_handlers[n] = handler;
}

