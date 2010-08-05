# kernel binary
kernel.bin:  init/loader-asm.o interrupts/idt-asm.o memory/gdt-asm.o common/generic.o devices/cpu/generic.o devices/display/generic.o devices/keyboard/generic.o devices/pit/generic.o init/main.o interrupts/idt.o interrupts/irq.o interrupts/isr.o memory/gdt.o
	ld -T linker.ld -nostdlib -o kernel.bin $^

# dependencies
common/generic.h:
devices/cpu/interface.h: common/generic.h
devices/display/interface.h: common/generic.h
devices/keyboard/interface.h: common/generic.h
devices/pit/interface.h: common/generic.h
interrupts/idt.h: common/generic.h
interrupts/irq.h: interrupts/isr.h
interrupts/isr.h: common/generic.h
memory/gdt.h: common/generic.h
common/generic.c: common/generic.h devices/display/interface.h
devices/cpu/generic.c: devices/cpu/interface.h
devices/display/generic.c: devices/display/interface.h
devices/keyboard/generic.c: devices/keyboard/interface.h devices/display/interface.h interrupts/irq.h
devices/pit/generic.c: devices/pit/interface.h interrupts/idt.h interrupts/irq.h devices/display/interface.h
init/main.c: common/generic.h devices/display/interface.h devices/cpu/interface.h devices/keyboard/interface.h memory/gdt.h interrupts/idt.h interrupts/irq.h devices/pit/interface.h
interrupts/idt.c: interrupts/idt.h devices/display/interface.h
interrupts/irq.c: interrupts/irq.h devices/display/interface.h
interrupts/isr.c: interrupts/isr.h devices/display/interface.h
memory/gdt.c: memory/gdt.h

# clean
clean:
	rm -rf kernel.bin init/loader-asm.o interrupts/idt-asm.o memory/gdt-asm.o common/generic.o devices/cpu/generic.o devices/display/generic.o devices/keyboard/generic.o devices/pit/generic.o init/main.o interrupts/idt.o interrupts/irq.o interrupts/isr.o memory/gdt.o



# how to compile .c to .o
%.o: %.c
	gcc -Wall -I . -ffreestanding -fno-stack-protector -o $@ -c $<

# how to compile file.asm to file-asm.o (rather than file.o because there exist c files with the same name, i.e. idt.c and and idt.asm would both correspond to idt.o)
%-asm.o: %.asm
	nasm -f elf -o $@ $<


run:
	qemu -kernel kernel.bin

test: kernel.bin run

makefile:
	tools/makefile.py
