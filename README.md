Xelix README
============

Xelix is an open source (GPL v3+ licensed) kernel, mainly for learning
how things work inside of a computer. It currently only supports x86.

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

    (./configure)
    (make clean)
    make

Steps in braces don't neccessarily have to be executed every time you
compile Xelix. If you're compiling the first time, run them.

Coding guidelines
-----------------

* Use tabs for indendations
* Opening curly brackets should go in the next line
* Put the pointer asterisk wherever you want. I prefer putting it to the type, but heck. Both versions have valid reasons for them.
* Globally callable functions should always be prefixed by their file/module name to avoid conflicts. Example: rtl8139_init().
	* An exemption can be made for commonly used library functions, especially if they're part of normal standard C libraries (Like strcmp, memcpy). However, those should be in src/lib/.
* Make internal functions static.
* Custom types are suffixed by _t.
	* Once again, exceptions can be made for stuff in src/lib/.
* If a struct is intended for global use, consider making it a type using typedef.
