#pragma once

#include <common/generic.h>


// should be called to init interrupts
// creates IDT et. al.
void interrupts_init();

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

typedef struct {
	uint32 ds;                  // Data segment selector
	uint32 edi, esi, ebp, esp, ebx, edx, ecx, eax; // Pushed by pusha.
	uint32 int_no, err_code;    // Interrupt number and error code (if applicable)
	uint32 eip, cs, eflags, useresp, ss; // Pushed by the processor automatically. // This is what the processor expects to be in the stack when doing an iret. useresp and ss are only used when returning to another privilege level
} registers_t;

typedef void (*interruptHandler_t)(registers_t); // interruptHandler_t is the type of a function with the signature  void func(registers_t)


// registers a interruptHandler (a callback which is called when the specified irq interrupt (ie n=IRQ1 for keyboard) occurs
void interrupt_registerHandler(uint8 n, interruptHandler_t handler);


// for internal use only!
void interrupt_callback(registers_t regs);
