#!/usr/bin/python2
import os;
import re;

# current working directory must be the main xelix directory with the Makefile inside!

makefile = open("Makefile", "w");

makefile.write("""
export LANG=C

# just issuing make will compile everything
all: xelix.bin

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
makefile.write("xelix.bin:");
for f in asmfiles:
	makefile.write(" " + f[:-4] + "-asm.o");
for f in cfiles:
	makefile.write(" " + f[:-2] + ".o");
makefile.write("\n\tld -melf_i386 -Ttext=0x100000 -nostdlib -o xelix.bin $^\n\n");

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
makefile.write("clean:\n\trm -rf xelix.bin mount floppy.img buildinfo.h ");
for f in asmfiles:
	makefile.write(" " + f[:-4] + "-asm.o");
for f in cfiles:
	makefile.write(" " + f[:-2] + ".o");

makefile.write("""\n\n

buildinfo.h:
	printf "#pragma once\\n" > buildinfo.h
	printf "#define __BUILDCOMP__ \\"`whoami`@`hostname`\\"\\n" >> buildinfo.h
	printf "#define __BUILDSYS__ \\"`uname -srop`\\"\\n" >> buildinfo.h
	printf "#define __BUILDDIST__ \\"`cat /etc/*-release | head -n 1 | xargs echo`\\"\\n" >> buildinfo.h

local.h:
	tools/txtconfig.py

config: clean local.h

# how to compile .c to .o
%.o: %.c
	gcc -frecord-gcc-switches -Wall -m32 -I . -ffreestanding -fno-stack-protector -o $@ -c $<

# how to compile file.asm to file-asm.o (rather than file.o because there exist c files with the same name, i.e. idt.c and and idt.asm would both correspond to idt.o)
%-asm.o: %.asm
	nasm -f elf -o $@ $<


makefile:
	tools/makefile.py

install: xelix.bin
	sudo cp xelix.bin /boot/xelix

# create a boot image for usb-stick or floppy
floppy.img: xelix.bin
	- mkdir mount
	cp tools/floppy.img .
	sudo losetup /dev/loop0 floppy.img
	sudo mount /dev/loop0 mount
	sudo cp xelix.bin mount/kernel
	sudo umount mount
	sudo losetup -d /dev/loop0
	- rm -rf mount

#create an iso
dist: floppy.img
	- mkdir mount
	cp tools/floppy.img .
	sudo losetup /dev/loop0 floppy.img
	sudo mount /dev/loop0 mount
	mkisofs -o xelix.iso mount
	sudo umount mount
	sudo losetup -d /dev/loop0
	- rm -rf mount


# running the kernel



runqemufloppy: floppy.img
	- rm /var/qemu.log
	qemu -d cpu_reset -monitor stdio -ctrl-grab -fda floppy.img

runbochsfloppy: floppy.img
	bochs -f bochsrc.txt -q

runqemu: xelix.bin
	qemu -d cpu_reset -monitor stdio -ctrl-grab -kernel xelix.bin

runqemunox: xelix.bin
	# Exit with ^A-x
	qemu -d cpu_reset -kernel xelix.bin -nographic
	
runvboxfloppy: floppy.img
	VBoxSDL -fda floppy.img --startvm Xelix




""");

makefile.close();

graphfile.write("}\n");
graphfile.close();


