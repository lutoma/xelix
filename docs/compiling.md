# Compiling
## Toolchain

To compile Xelix, you first need the Xelix toolchain. It consists of binutils, a GCC cross-compiler and the newlib C standard library, all with configuration and patches to support the i786-pc-xelix target.

### Dependencies

Please check [Prerequisites for GCC](https://gcc.gnu.org/install/prerequisites.html) for GCC build dependencies.

Since we are patching the packages, you will also need autoconf/automake. On Debian-based systems, you can install the dependencies using `àpt install build-essential autoconf automake`.

Some of the packages involved, particularly binutils and newlib, are very picky about autoconf/automake versions. In addition to the regular versions of autoconf and automake, you will need:

	* autoconf 2.69
	* automake 1.11

The toolchain Makefile expects the binaries to be available in your `PATH` as suffixed versions (e.g. `autoconf-2.69`). Some distributions have these available through package management, otherwise you can build them using

	#!bash
	wget https://ftp.gnu.org/gnu/autoconf/autoconf-2.69.tar.gz
	tar xf autoconf-2.69.tar.gz
	cd autoconf-2.69
	./configure --program-suffix=-2.69
	make
	sudo make install

	wget https://ftp.gnu.org/gnu/automake/automake-1.11.6.tar.xz
	tar xf automake-1.11.6.tar.xz
	cd automake-1.11.6/
	./configure --program-suffix=-1.11
	make
	sudo make install


### Building the toolchain

Once you have the dependencies, you can build the toolchain using

	#!bash
	make -C toolchain

Depending on your hardware, this will take a significant amount of time.

Once it is done, you should find Xelix-specific binaries of GCC, g++, ld, etc. in `toolchain/local/bin`. I find it convenient to add that directory to `PATH`, but the Xelix Makefile will also work without it.

`i786-pc-xelix-gcc` and `i786-pc-xelix-g++` from that directory can also be used to compile userspace binaries and automatically include the correct standard library, linker script and crt0:

	#!bash
	# Can be used like regular GCC
	i786-pc-xelix-gcc test.c -o test

## Compiling Xelix

In addition to the toolchain, the [NASM](https://www.nasm.us/) assembler is also required. Since no Xelix-specific patches are needed, you can just use the one from your distribution's package sources (Arch Linux: `pacman -S nasm`, Debian/Ubuntun: `Apt install nasm`).

Once you have that in place, you can compile Xelix using:

	#!bash
	# Optional, if you want to customize settings:
	make menuconfig

	./configure
	make

You should now see a binary called `xelix.bin`. 🎉 Place it in the `/boot` directory of your Xelix installation and reboot (Note that `/boot` may be on a separate partition).

