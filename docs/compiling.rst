Compiling Xelix
***************

Toolchain
=========

To compile Xelix, you first need the Xelix toolchain. It consists of binutils,
a GCC cross-compiler and the newlib C standard library, all with configuration
and patches to support the i786-pc-xelix target.

You can build the toolchain using

.. code-block:: shell

   make -C toolchain

Depending on your hardware, this will take a significant amount of time.

Once it is done, you should find Xelix-specific binaries of GCC, g++, ld, etc. in :file:`toolchain/local/bin`. I find it convenient to add that directory to `PATH`, but the Xelix Makefile will also work without doing so.

`i786-pc-xelix-gcc` and `i786-pc-xelix-g++` from that directory can also be used to compile userspace binaries and automatically include the correct standard library, linker script and crt0:

.. code-block:: shell

   # Can be used like regular GCC
   i786-pc-xelix-gcc test.c -o test

Compiling
=========

In addition to the toolchain, the `NASM <https://www.nasm.us/>`_ assembler
also needs to be installed.

Once you have that in place, you can compile Xelix using:

.. code-block:: shell

   make gconfig
   ./configure
   make

You should now see a binary called `xelix.bin`. 🎉 Place it in the :file:`/boot` directory of your Xelix installation and reboot (Note that :file:`/boot` may be on a separate partition).

