# Interrupts

x86 interrupt gates are set up during early startup in `int/i386-idt.c`. Since x86 interrupts do not pass their number in a register, we need to set each interrupt gate to a different handler so we can figure out which interrupt was invoked. The handlers for each interrupt gate are generated in `src/int/i386-interrupts.asm` using a preprocessor loop.

Some interrupts/exceptions also automatically push an additional error code on the stack. To make sure we can store the data for interrupts that don't push an error code in the same struct, their handlers push a placeholder 0 here.

Afterwards, these interrupt-specific handlers pass control to the generic assembly interrupt handler `int_i386_dispatch`, which in turn invokes the C interrupt handler `int_dispatch`.

## Context switching
