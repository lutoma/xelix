#ifndef INTERRUPTS_IDT_H
#define INTERRUPTS_IDT_H

#include <common/generic.h>

#define IRQ0 32
#define IRQ1 33
#define IRQ2 34
#define IRQ3 35
#define IRQ4 36
#define IRQ5 37
#define IRQ6 38
#define IRQ7 39
#define IRQ8 40
#define IRQ9 41
#define IRQ10 42
#define IRQ11 43
#define IRQ12 44
#define IRQ13 45
#define IRQ14 46
#define IRQ15 47

typedef struct registers
{
	uint32 ds;						// Data segment selector
	uint32 edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
	uint32 int_no, err_code;	 // Interrupt number and error code (if applicable)
	uint32 eip, cs, eflags, useresp, ss; // Pushed by the processor automatically.
} registers_t;

typedef void (*isr_t)(registers_t);

void idt_init();
void idt_registerHandler(uint8 n, isr_t handler);
#endif
