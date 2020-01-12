Interrupts
**********

IDT initialization
==================

During early start, all 255 interrupt gates are set to point at small wrapper functions defined in :file:`src/int/i386-interrupts.asm` using a preprocessor loop. These store the interrupt number as well as an error code, if any, then pass control to the generic assembly interrupt handler `interrupts_common_handler`.

Context switching
=================
