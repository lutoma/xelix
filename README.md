# Xelix kernel

Xelix is a hobby POSIX kernel and operating system for x86. It has a largely GNU-based userland and can run many common \*nix programs.

Releases, build instructions and code documentation can be found at https://xelix.org (or in the `docs` folder).

## Features

* Preemptive multitasking with privilege and memory separation
* POSIX process API with fork/execve/wait and signal handling
* VFS with support for dynamic mount points, poll, pipes, and in-memory file trees
* Read/write ext2 implementation, IDE and virtio-block drivers
* BSD socket API for TCP/IP support using the [PicoTCP network stack](https://github.com/tass-belgium/picotcp)
* Terminal framework with support for multiple TTYs, pseudo terminals & ECMA48 escape sequences
