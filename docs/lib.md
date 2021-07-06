# Kernel standard library

Xelix has a minimal standard library in `src/lib`. It implements a subset of C stdlib functionality as well as a number of Xelix-specific functions.

The lib directory is part of the include path, so `src/lib/string.h` can be included using `#include <string.h>`.

## Function overview

Header     | Functions
-----------|----------
bitmap.h   | bit_set, bit_clear, bit_toggle, bit_get, bitmap_index, bitmap_offset, bitmap_size, bitmap_get, bitmap_set, bitmap_clear, bitmap_find, bitmap_count
cmdline.h  | cmdline_get, cmdline_get_bool
endian.h   | endian_swap16, endian_swap32, endian_swap64
errno.h    | sc_errno
[kavl.h](https://github.com/attractivechaos/klib) | kavl_insert, kavl_find, kavl_erase, kavl_erase_first, kavl_itr_first, kavl_itr_find, kavl_itr_next, kavl_at, KAVL_INIT, KAVL_INIT2
panic.h    | assert, assert_nc, addr2name, panic
[printf.h](https://github.com/mpaland/printf) | printf, sprintf, snprintf, vsnprintf, vprintf, fctprintf
spinlock.h | spinlock_release, spinlock_cmd, spinlock_get
stdlib.h   | atoi, is_digit
string.h   | strdup, strcmp, strcasecmp, strncasecmp, strncmp, strcat, strcpy, strncpy, strlen, strndup, memset, memcpy, memcmp, memmove, strchr, bzero, strtok_r, substr, find_substr, asprintf, memset32
time.h     | time_get, time_get_timeval, sleep, uptime
variadic.h | variadic_call

## Kernel logger

`src/lib/log.c` contains the kernel logger. It outputs messages to display and serial and stores them in a kernel buffer. Output can also be configured depending on severity.

```c
void log(uint32_t level, const char *fmt, ...);
```

Available log levels: `LOG_DEBUG`, `LOG_INFO`, `LOG_WARN`, `LOG_ERR`. Debug messages are not stored in the kernel buffer.

Note that before kmalloc is initalized, the logger uses a fixed-size buffer of 0x1000 bytes which can run out. It switches to an automatically growing dynamic buffer after memory initialization.

On a running Xelix system, log messages can be inspected using the xelix-utils `dmesg` tool. They can also be read directly from `/sys/log` with raw timestamps.

## Kernel command line

`src/lib/cmdline.c` contains a simple kernel command line parser. It retrieves a command line of the format `root=/dev/ide1p1 init=/usr/bin/bash` from Multiboot and makes the individual entries available using


```c
char* cmdline_get(const char* key);
bool cmdline_get_bool(const char* key);
```
