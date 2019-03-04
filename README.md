# 💻 Xelix

Xelix is a hobbyist monolithic x86 kernel, mainly written for learning
purposes. It aims to conform to the POSIX specification.

![](https://fnord.cloud/s/E63wpFwEzBBSr9G/preview)

## ✨ Features

  * Preemptive multi-tasking with privilege and memory separation
  * POSIX process API with fork/execve/wait and signal handling
  * Read/write ext2 implementation
  * TCP/IP support using the [PicoTCP](https://github.com/tass-belgium/picotcp) network stack
  * TTY framework with support for ECMA48 escape sequences

Ported software: GNU coreutils, bash, dash, pciutils, bzip2/zlib/gzip, ncurses, dialog,
nano, darkhttpd, [and more](https://github.com/lutoma/xelix/tree/master/land).

## 🏃 Running Xelix

An disk image containing Xelix and a number of utilities is available [on the
releases page](https://github.com/lutoma/xelix/releases/download/v20190126/xelix-2019-01-26.qcow2).

You should be able to run it using any common x86 emulator/hypervisor.
QEMU/KVM, Bochs, VirtualBox and VMWare Player are known to work, with the last
two offering the best performance (Most testing is done on QEMU however).

Sample QEMU invocation:

	qemu-system-i386 -accel kvm -hda xelix.img -m 512 -cpu SandyBridge -soundhw ac97

## ⚙ Compiling

Compiling Xelix requires a Xelix toolchain with versions of GCC, binutils
and newlib that accept the `i786-pc-xelix` architecture and have the correct
syscall implementations.

You can build the toolchain using

	make -C toolchain

Depending on your hardware, this is going to take a _long_ time. In addition to
the toolchain, [NASM](https://www.nasm.us/) also needs to be installed.
Afterwards you should be able to compile xelix using:

    make gconfig
    ./configure
    make

You should now see a binary called `xelix.bin`. 🎉

## 🖼 Building an image

You can build a full Xelix system image using `make image`. This requires an
existing Xelix kernel binary and toolchain. Since this compiles all userland
libraries and executables, this will also take a long time.
