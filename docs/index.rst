Xelix kernel
************

Xelix is a hobby POSIX kernel with GNU userland for x86.

.. raw:: html

   <link type="text/css" rel="stylesheet" href="_static/css/lightslider.css" />
   <script src="_static/js/lightslider.min.js"></script>

   <ul id="slider">
      <li data-thumb="_static/img/screenshot1.png">
         <img src="_static/img/screenshot1.png">
      </li>
      <li data-thumb="https://camo.githubusercontent.com/520f34a3962e5358c0885fb29da9891fd5993ced/68747470733a2f2f666e6f72642e636c6f75642f732f45363377704677457a4242537239472f70726576696577">
         <img src="https://camo.githubusercontent.com/520f34a3962e5358c0885fb29da9891fd5993ced/68747470733a2f2f666e6f72642e636c6f75642f732f45363377704677457a4242537239472f70726576696577">
      </li>
   </ul>

   <script>
      $(document).ready(function() {
         $('#slider').lightSlider({
            gallery: true,
            item: 1,
            loop: true,
            slideMargin: 0,
            thumbItem: 9
         });
      });
   </script>

✨ Features
===========

* Preemptive multitasking with privilege and memory separation
* POSIX process API with fork/execve/wait and signal handling
* VFS with support for poll, pipes, and in-memory file trees
* Read/write ext2 implementation, IDE and virtio-block drivers
* BSD socket API for TCP/IP support using the `PicoTCP network stack <https://github.com/tass-belgium/picotcp>`_
* TTY framework with support for multiple TTYs, pseudo terminals & ECMA48 escape sequences

Releases
========

Xelix is unstable pre-alpha software. **Don't use it on computers that store data you don't want to lose.** Make liberal use of `fsck.ext2`.

+-------------------+--------------------------------------------+-----------------------------------------------------------+
| Release           | Download                                   | New software                                              |
+===================+============================================+===========================================================+
| 2020-01-10        | .qcow2 disk image                          | coreutils, bash, binutils, gcc, vim, wget, dropbear,      |
|                   |                                            | xz, flac, pciutils, pcre2, htop, openssl, gmp, mpfr, mpc, |
|                   |                                            | openssl, xelix-utils, dialog                              |
+-------------------+--------------------------------------------+-----------------------------------------------------------+
| 2019-01-26        | `.qcow2 disk image                         | busybox, xshell, bzip2, zlib, ncurses, diffutils, grep,   |
|                   | </releases/xelix-2019-01-26.qcow2>`_       |                                                           |
+-------------------+--------------------------------------------+-----------------------------------------------------------+

You should be able to run it using any common x86 emulator/hypervisor.
QEMU/KVM, Bochs, VirtualBox and VMWare Player are known to work, with the
latter two offering the best performance (Most testing is done on QEMU/KVM however).

Sample QEMU invocation:

.. code-block:: shell

   qemu-system-i386 -accel kvm -hda xelix.qcow2 -m 1024 -cpu SandyBridge -soundhw ac97

Documentation
=============

.. toctree::
   :maxdepth: 2

   compiling
   boot
   fs
   mem
   int
   tasks
   tty
   syscalls
   lib
   xelix-utils



Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
