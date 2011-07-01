Xelix README
============

Xelix is an open source (GPL v3+ licensed) kernel, mainly for learning
how things work inside of a computer. It currently only supports x86.

Dependencies
------------

 * GNU Make
 * GNU GCC (>= 4.4.3)
 * NASM
 * GNU Binutils
 * Python 2 (>= 2.6)
 
Also, the following libraries are being used (Git fetches them
automatically, but for the sake of completeness, they're still listed
here):

 * Libkdict
 * Libstrbuffer

Compiling
---------

 * (git submodule update --init)
 * (./configure)
 * (make clean)
 * make

Steps in braces don't neccessarily have to be executed every time you
compile Xelix. If you're compiling the first time, run them.
