# kernel binary
kernel: init/loader.o memory/gdta.o interrupts/idta.o  common/generic.o devices/cpu/generic.o devices/display/generic.o devices/keyboard/generic.o init/main.o interrupts/idt.o interrupts/pit.o memory/gdt.o
	ld -T linker.ld -o kernel.bin $^

# dependencies
common/generic.h:
devices/cpu/interface.h: common/generic.h
devices/display/interface.h: common/generic.h
devices/keyboard/interface.h: common/generic.h
interrupts/idt.h: common/generic.h
interrupts/pit.h: common/generic.h
memory/gdt.h: common/generic.h
common/generic.c: common/generic.h devices/display/interface.h
devices/cpu/generic.c: devices/cpu/interface.h
devices/display/generic.c: devices/display/interface.h
devices/keyboard/generic.c: devices/keyboard/interface.h devices/display/interface.h
init/main.c: common/generic.h devices/display/interface.h devices/cpu/interface.h devices/keyboard/interface.h memory/gdt.h interrupts/idt.h interrupts/pit.h
interrupts/idt.c: interrupts/idt.h devices/display/interface.h
interrupts/pit.c: interrupts/pit.h interrupts/idt.h devices/display/interface.h
memory/gdt.c: memory/gdt.h

# clean
clean:
	rm -rf kernel.bin init/loader.o common/generic.o devices/cpu/generic.o devices/display/generic.o devices/keyboard/generic.o init/main.o interrupts/idt.o interrupts/pit.o memory/gdt.o


# how to compile .c to .o
.c.o:
	gcc -Wall -I . -nostartfiles -nodefaultlibs -nostdlib -fno-stack-protector -o $@ -c $<


init/loader.o: init/loader.asm
	nasm -f elf -o init/loader.o init/loader.asm
memory/gdta.o: memory/gdt.asm
	nasm -f elf -o memory/gdta.o memory/gdt.asm
interrupts/idta.o: interrupts/idt.asm
	nasm -f elf -o interrupts/idta.o interrupts/idt.asm

run:
	qemu -kernel kernel.bin

test: kernel run

makefile:
	tools/makefile.py
