Xelix README
============

Xelix is an open source (GPL v3+ licensed) kernel, mainly for learning
how things work inside of a computer. It currently only supports x86.

![A screenshot of Xelix running dash](https://i.imgur.com/v7S6U1t.png)

You'll need
------------

 * GNU/BSD Make
 * GNU CC (>= 4.4.3)
 * NASM
 * GNU Binutils

On OS X, you'll need to install the following packages using Homebrew:
`gcc findutils binutils nasm`

Compiling
---------

    make menuconfig (or xconfig, gconfig, config)
    ./configure
    make
