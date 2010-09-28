#pragma once

#include <common/generic.h>


// build the idt (also remaps irqs to isr)
void idt_init();
