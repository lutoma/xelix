# Xelix kernel

Xelix is a hobby Unix-like kernel and operating system for x86. It has a largely GNU-based userland and can run many common Linux/BSD programs.

[![Screenshot of Xelix 0.3.0-alpha](images/screenshot.png)](images/screenshot.png)

## Features

* Preemptive multitasking with privilege and memory separation
* Unix process API with fork/execve/wait and signal handling
* VFS with support for dynamic mount points, poll, pipes, and in-memory file trees
* Read/write ext2 implementation, IDE and virtio-block drivers
* BSD socket API for TCP/IP support using the [PicoTCP network stack](https://github.com/tass-belgium/picotcp)
* Terminal framework with support for multiple TTYs, pseudo terminals & ECMA48 escape sequences

## Current development goals

* Support for memory-mapping in userspace using mmap and general memory allocation overhaul
* Dynamic library loading
* Graphics compositing improvements and proper input handling for GUI tasks
* Virtual file system overhaul to cache file tree in memory and do lazy lookups only

## Releases


!!! danger
	Xelix is unstable pre-alpha software. **Don't use it on computers that store any data you might want to keep.** Make liberal use of `fsck.ext2`.

 Version       | Release           | Download                                   | New software
---------------|-------------------|--------------------------------------------|--------------
 0.3.0-alpha   | 2021-07-07        | [QEMU qcow2 image](https://github.com/lutoma/xelix/releases/download/0.3.0-alpha/xelix-0.3.0-alpha.qcow2)                | gfxcompd, gfxterm, libexpat, pixman, libiconv, cairo, libpng, tz, htop
 0.2.0         | 2020-01-26        | [QEMU qcow2 image](https://github.com/lutoma/xelix/releases/download/v0.2.0/xelix-0.2.0.qcow2)                           | dash, dialog, flac, gdbm, libedit, nano, openssl, tar, which, xz, bash, coreutils, freetype2, gettext, libtextstyle, mpc, pciutils, vim, binutils, darkhttpd, e2fsprogs, gmp, less, libxml2, mpfr, pcre2, wget
 0.1.0         | 2019-01-26        | [QEMU qcow2 image](https://github.com/lutoma/xelix/releases/download/v0.1.0/xelix-0.1.0.qcow2)                           | busybox, xshell, bzip2, zlib, ncurses, diffutils, grep

You should be able to run it using any common x86 emulator/hypervisor.
QEMU/KVM, Bochs, VirtualBox and VMWare Player are known to work, with the
latter two offering the best performance (Most testing is done on QEMU/KVM however).

Sample QEMU invocation:

```bash
qemu-system-i386 -accel kvm -vga qxl -m 1024 -cpu host -serial mon:stdio -device ac97 -drive file=xelix.qcow2,if=ide -netdev user,id=mnet0 -device virtio-net,netdev=mnet0
```
