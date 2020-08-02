# Compiling
## Toolchain

To compile Xelix, you first need the Xelix toolchain. It consists of binutils, a GCC cross-compiler and the newlib C standard library, all with configuration and patches to support the i786-pc-xelix target.

Please check [Prerequisites for GCC](https://gcc.gnu.org/install/prerequisites.html) for GCC build dependencies. You will also need autoconf/automake. On Debian-based systems, you can install the dependencies using `àpt install build-essential autoconf automake`.

You can build the toolchain using

	#!bash
	make -C toolchain

Depending on your hardware, this will take a significant amount of time.

Once it is done, you should find Xelix-specific binaries of GCC, g++, ld, etc. in `toolchain/local/bin`. I find it convenient to add that directory to `PATH`, but the Xelix Makefile will also work without doing so.

`i786-pc-xelix-gcc` and `i786-pc-xelix-g++` from that directory can also be used to compile userspace binaries and automatically include the correct standard library, linker script and crt0:

	#!bash
	# Can be used like regular GCC
	i786-pc-xelix-gcc test.c -o test

## Compiling

In addition to the toolchain, the [NASM](https://www.nasm.us/) assembler is also required. Since no Xelix-specific patches are needed, you can just use the one from your distribution's package sources (Arch Linux: `pacman -S nasm`, Debian/Ubuntun: `Apt install nasm`).

Once you have that in place, you can compile Xelix using:

	#!bash
	make gconfig
	./configure
	make

You should now see a binary called `xelix.bin`. 🎉 Place it in the `/boot` directory of your Xelix installation and reboot (Note that `/boot` may be on a separate partition).

