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

 Version | Release           | Download                                   | New software
---------|-------------------|--------------------------------------------|--------------
 0.2.0   | 2020-01-26        | [QEMU qcow2 image](https://github.com/lutoma/xelix/releases/download/v0.2.0/xelix-0.2.0.qcow2)                           | dash, dialog, flac, gdbm, libedit, nano, openssl, tar, which, xz, bash, coreutils, freetype2, gettext, libtextstyle, mpc, pciutils, vim, binutils, darkhttpd, e2fsprogs, gmp, less, libxml2, mpfr, pcre2, wget
 0.1.0   | 2019-01-26        | [QEMU qcow2 image](https://github.com/lutoma/xelix/releases/download/v0.1.0/xelix-0.1.0.qcow2)                           | busybox, xshell, bzip2, zlib, ncurses, diffutils, grep

You should be able to run it using any common x86 emulator/hypervisor.
QEMU/KVM, Bochs, VirtualBox and VMWare Player are known to work, with the
latter two offering the best performance (Most testing is done on QEMU/KVM however).

Sample QEMU invocation:

```bash
qemu-system-i386 -accel kvm -vga qxl -m 1024 -cpu SandyBridge -soundhw ac97 -drive file=xelix-0.2.0.qcow2,if=ide -netdev user,id=mnet0 -device virtio-net,netdev=mnet0
```
