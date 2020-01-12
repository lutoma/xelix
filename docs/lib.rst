Library functions
*****************

Xelix has a very minimal "C standard library" in :file:`src/lib`. This directory is part of the include path, so :file:`src/lib/printf.h` can be included using `#include <printf.h>`. The files loosely follow standard library naming.

Kernel logger
=============

:file:`src/lib/log.c` contains the kernel logger. It outputs messages to display and serial and stores them in a kernel buffer. Output can also be configured depending on severity. On a running Xelix system, log messages can be retrieved using the xelix-utils `dmesg` tool (which reads from :file:`/sys/log`).

 .. code-block:: c

   void log(uint32_t level, const char *fmt, ...);

Available log levels: LOG_DEBUG, LOG_INFO, LOG_WARN, LOG_ERR. Debug messages are not stored in the kernel buffer.

.. _kernel-command-line:

Kernel command line
===================

:file:`src/lib/cmdline.c` contains a simple kernel command line parser. It retrieves a command line of the format `root=/dev/ide1p1 init=/usr/bin/bash` from Multiboot and makes the individual entries available using

 .. code-block:: c

   char* cmdline_get(const char* key);
   bool cmdline_get_bool(const char* key);
