#!/usr/bin/python
import os;
import re;

# current working directory must be the main xelix directory with the Makefile inside!

makefile = open("Makefile", "w");

makefile.write("""
export LANG=C

# just issuing make will compile everything
all: kernel.bin initrd.img

\n""");


# visualisation of includes! (open eg. with kgraphviewer)

graphfile = open("includesgraph.dot", "w");
graphfile.write("digraph unnamed {\n");


cfiles = [];
hfiles = [];
asmfiles = []

for root, dirs, files in os.walk("."):
	if ".git" in root or "tools" in root:
		continue
	for f in files:
		dateiname = (root + "/" + f)[2:];
		if dateiname[-2:] == ".c":
			cfiles.append(dateiname);
		if dateiname[-2:] == ".h":
			hfiles.append(dateiname);
		if dateiname[-4:] == ".asm":
			asmfiles.append(dateiname);

cfiles.sort();
hfiles.sort();
asmfiles.sort();


makefile.write("# kernel binary\n");
makefile.write("kernel.bin:");
for f in asmfiles:
	makefile.write(" " + f[:-4] + "-asm.o");
for f in cfiles:
	makefile.write(" " + f[:-2] + ".o");
makefile.write("\n\tld -T linker.ld -nostdlib -o kernel.bin $^\n\n");

makefile.write("# dependencies\n");
for f in hfiles + cfiles:
	makefile.write(f + ":");
	fread = open(f, "r");
	for line in fread:
		m = re.search("#include <(.+)>", line);
		if m != None:
			makefile.write(" " + m.group(1));
			graphfile.write('"' + m.group(1) + '" -> "' + f + '"\n');
	makefile.write("\n");

makefile.write("\n# clean\n");
makefile.write("clean:\n\trm -rf kernel.bin mount initrd.img floppy.img");
for f in asmfiles:
	makefile.write(" " + f[:-4] + "-asm.o");
for f in cfiles:
	makefile.write(" " + f[:-2] + ".o");

makefile.write("""\n\n

# how to compile .c to .o
%.o: %.c
	gcc -Wall -Werror -I . -ffreestanding -fno-stack-protector -o $@ -c $<

# how to compile file.asm to file-asm.o (rather than file.o because there exist c files with the same name, i.e. idt.c and and idt.asm would both correspond to idt.o)
%-asm.o: %.asm
	nasm -f elf -o $@ $<


# initrd image
initrd.img: tools/makeinitrd
	tools/makeinitrd tools/test.txt test.txt tools/helloworld helloworld.bin
tools/makeinitrd: tools/makeinitrd.c
	gcc -o tools/makeinitrd tools/makeinitrd.c


makefile:
	tools/makefile.py


# create a boot image for usb-stick or floppy
floppy.img: kernel.bin initrd.img
	- mkdir mount
	cp tools/floppy.img .
	sudo losetup /dev/loop0 floppy.img
	sudo mount /dev/loop0 mount
	sudo cp kernel.bin mount/kernel
	sudo cp initrd.img mount/initrd
	sudo umount mount
	sudo losetup -d /dev/loop0
	- rm -rf mount



# running the kernel



runqemufloppy: floppy.img
	- rm /var/qemu.log
	# qemu -initrd doesn't work as it should.. (more precise, please!)
	qemu -d cpu_reset -monitor stdio -ctrl-grab -fda floppy.img

runbochsfloppy: floppy.img
	bochs -f bochsrc.txt -q

runqemu: initrd.img kernel.bin
	qemu -d cpu_reset -monitor stdio -ctrl-grab -kernel kernel.bin -initrd initrd.img

runvboxfloppy: floppy.img
	VBoxSDL -fda floppy.img --startvm Xelix




""");

makefile.close();

graphfile.write("}\n");
graphfile.close();


