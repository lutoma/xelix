Xelix README
============

Xelix is an open source (GPL v3+ licensed) kernel, mainly for learning
how things work inside of a computer. It currently only supports x86.

You'll need
------------

 * GNU Make
 * GNU CC (>= 4.4.3)
 * NASM
 * GNU Binutils
 
Compiling
---------

::

    (./configure)
    (make clean)
    make

Steps in braces don't neccessarily have to be executed every time you
compile Xelix. If you're compiling the first time, run them.

Where to submit Bugs / Tasks / Suggestions?
-------------------------------------------

See http://xelix.org/trac/query for the most current list

... does it run on my version of QEMU / VirtualBox / Laptop / Toaster...?
-------------------------------------------------------------------------

See http://xelix.org/trac/wiki/HardwareSupport. If your host system isn't in the list, please consider testing and adding it.

Coding guidelines
-----------------

* Use tabs for indendations
* Opening curly brackets go in the next line
	* An exemption can be made for struct definitions. There is no real reason for this, but i'm used to it and have always done it that way.
* The pointer * is part of the type. Therefore, it's void* bla, not void *bla.
* Don't insert an useless space after builtin functions like if or for.
* Globally callable functions should always be prefixed by their file/module name to avoid conflicts. Example: rtl8139_init().
	* An exemption can be made for commonly used library functions, especially if they're part of normal standard C libraries (Like strcmp, memcpy). However, those should be in src/lib/.
* Make internal functions static.
* Custom types are suffixed by _t.
	* Once again, exceptions can be made for stuff in src/lib/.
* If a struct is intended for global use, consider making it a type using typedef.

