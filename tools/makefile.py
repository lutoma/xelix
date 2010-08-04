#!/usr/bin/python
import os;
import re;

# current working directory must be the main decore directory with the Makefile inside!

makefile = open("Makefile", "w");

cfiles = [];
hfiles = [];

for root, dirs, files in os.walk("."):
	if ".git" in root:
		continue
	for f in files:
		dateiname = (root + "/" + f)[2:];
		if dateiname[-2:] == ".c":
			cfiles.append(dateiname);
		if dateiname[-2:] == ".h":
			hfiles.append(dateiname);

cfiles.sort();
hfiles.sort();


makefile.write("# kernel binary\n");
makefile.write("kernel: init/loader.o");
for f in cfiles:
	makefile.write(" " + f[:-2] + ".o");
makefile.write("\n\tld -T linker.ld -o kernel.bin $^\n\n");

makefile.write("# dependencies\n");
for f in hfiles + cfiles:
	makefile.write(f + ":");
	fread = open(f, "r");
	for line in fread:
		m = re.search("#include <(.+)>", line);
		if m != None:
			makefile.write(" " + m.group(1));
	makefile.write("\n");

makefile.write("\n# clean\n");
makefile.write("clean:\n\trm -rf kernel.bin init/loader.o");
for f in cfiles:
	makefile.write(" " + f[:-2] + ".o");

makefile.write("""\n\n
# how to compile .c to .o
.c.o:
	gcc -Wall -I . -nostartfiles -nodefaultlibs -nostdlib -o $@ -c $<


init/loader.o: init/loader.s
	nasm -f elf -o init/loader.o init/loader.s


run: kernel
	qemu -kernel kernel.bin

makefile:
	tools/makefile.py
""");




makefile.close();

