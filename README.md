Xelix README
============

Xelix is a hobbyist monolithic x86 kernel, mainly written for learning
purposes. It aims to conform to the POSIX specification.

It features multi-tasking with privilege and memory separation and an
implementation of the ext2 file system, as well as hardware drivers
for the RTL8139 NIC and the AC97 sound chip.

Xelix uses newlib as userland standard C library. Ported software includes
busybox, dash, pciutils, bzip2/zlib/gzip, and the PicoTCP userland network
stack.

[📷 Xelix running in QEMU](https://fnord.cloud/s/ATe9C96YC75wG5J/preview)

Compiling
---------

Compiling Xelix requires a Xelix toolchain with patched versions of GCC,
binutils and newlib that accept the `i786-pc-xelix` architecture and have
the correct syscall implementations.

You can build the toolchain using

	make -C toolchain

depending on your hardware, this is going to take a _long_ time. In addition to
the toolchain, [NASM](https://www.nasm.us/) also needs to be installed.

Afterwards you should be able to compile xelix using:

    make gconfig (or menuconfig, config, xconfig)
    ./configure
    make

You should now see a binary called `xelix.bin`. 🎉
