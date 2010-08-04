# kernel binary
kernel: init/loader.o common/generic.o devices/display/generic.o init/main.o
	ld -T linker.ld -o kernel.bin $^

# dependencies
common/generic.h:
devices/display/interface.h: common/generic.h
common/generic.c: common/generic.h devices/display/interface.h
devices/display/generic.c: devices/display/interface.h
init/main.c: common/generic.h devices/display/interface.h

# clean
clean:
	rm -rf kernel.bin init/loader.o common/generic.o devices/display/generic.o init/main.o


# how to compile .c to .o
.c.o:
	gcc -Wall -I . -nostartfiles -nodefaultlibs -nostdlib -o $@ -c $<


init/loader.o: init/loader.s
	nasm -f elf -o init/loader.o init/loader.s


run: kernel
	qemu -kernel kernel.bin

makefile:
	tools/makefile.py
