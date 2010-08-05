#ifndef INTERRUPTS_IRQ_H
#define INTERRUPTS_IRQ_H

#include <interrupts/isr.h>



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

typedef void (*irqHandler_t)(registers_t);

// registers a irqHandler (a callback which is called when the specified irq interrupt (ie n=IRQ1 for keyboard) occurs
void irq_registerHandler(uint8 n, irqHandler_t handler);

#endif
