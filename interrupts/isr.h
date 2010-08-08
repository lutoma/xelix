#ifndef INTERRUPTS_ISR_H
#define INTERRUPTS_ISR_H

#include <common/generic.h>

typedef struct
{
   uint32 ds;                  // Data segment selector
   uint32 edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
   uint32 int_no, err_code;    // Interrupt number and error code (if applicable)
   uint32 eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
} registers_t;

typedef void (*interruptHandler_t)(registers_t); // interruptHandler_t is the type of a function with the signature  void func(registers_t)

// registers a interruptHandler (a callback which is called when the specified interrupt (ie n=IRQ1 for keyboard) occurs
void isr_registerHandler(uint8 n, interruptHandler_t handler);

#endif
