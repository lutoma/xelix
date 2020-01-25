# Xelix kernel

Xelix is a hobby POSIX kernel and operating system for x86. It has a largely GNU-based userland and can run many common \*nix programs.

## Features

* Preemptive multitasking with privilege and memory separation
* POSIX process API with fork/execve/wait and signal handling
* VFS with support for dynamic mount points, poll, pipes, and in-memory file trees
* Read/write ext2 implementation, IDE and virtio-block drivers
* BSD socket API for TCP/IP support using the [PicoTCP network stack](https://github.com/tass-belgium/picotcp)
* Terminal framework with support for multiple TTYs, pseudo terminals & ECMA48 escape sequences

## Releases


!!! danger
	Xelix is unstable pre-alpha software. **Don't use it on computers that store any data you might want to keep.** Make liberal use of `fsck.ext2`.

 Release           | Download                                   | New software
-------------------|--------------------------------------------|--------------
 2020-01-10        | QEMU qcow2 image                           | coreutils, bash, binutils, gcc, vim, wget, dropbear, xz, flac, pciutils, pcre2, htop, openssl, gmp, mpfr, mpc, xelix-utils, dialog
 2019-01-26        | [QEMU qcow2 image](https://github.com/lutoma/xelix/releases/download/v20190126/xelix-2019-01-26.qcow2)                           | busybox, xshell, bzip2, zlib, ncurses, diffutils, grep

You should be able to run it using any common x86 emulator/hypervisor.
QEMU/KVM, Bochs, VirtualBox and VMWare Player are known to work, with the
latter two offering the best performance (Most testing is done on QEMU/KVM however).

Sample QEMU invocation:

	#!bash
	qemu-system-i386 -accel kvm -hda xelix.qcow2 -m 1024 -cpu SandyBridge -soundhw ac97

