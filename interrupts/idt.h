#ifndef INTERRUPTS_IDT_H
#define INTERRUPTS_IDT_H

#include <common/generic.h>


// build the idt (also remaps irqs to isr)
void idt_init();


#endif
