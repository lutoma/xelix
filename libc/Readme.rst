Xelix libc
==========

This is the libc of the Xelix kernel.

Usage
=====

Firstly, compile using ./configure and make. Then, link your program
with the following parameters:

ld -nostdlib -nostdinc -Iinclude -L. -melf_i386 -o <your_program> <your_objectfiles> -lstdc

Status
======

The following header files are considered complete (As of the The Open 
Group Base Specifications Issue 6 [Better known as POSIX]):

<stddef.h>
<errno.h>
<sys/utsname.h>
<stdbool.h>
<iso646.h>
<tar.h>

The following header files are partially implemented:

<stdint.h>
<stdio.h>
<stdlib.h>
<string.h>
<unistd.h>
<stdarg.h>
<sys/types.h>
<inttypes.h>
<limits.h>
<sys/stat.h>
<time.h>
<setjmp.h>

The following header files are completely missing:

<aio.h>
<arpa/inet.h>
<assert.h>
<complex.h>
<cpio.h>
<ctype.h>
<dirent.h>
<dlfcn.h>
<fcntl.h>
<fenv.h>
<float.h>
<fmtmsg.h>
<fnmatch.h>
<ftw.h>
<glob.h>
<grp.h>
<iconv.h>
<langinfo.h>
<libgen.h>
<locale.h>
<math.h>
<monetary.h>
<mqueue.h>
<ndbm.h>
<net/if.h>
<netdb.h>
<netinet/in.h>
<netinet/tcp.h>
<nl_types.h>
<poll.h>
<pthread.h>
<pwd.h>
<regex.h>
<sched.h>
<search.h>
<semaphore.h>
<setjmp.h>
<signal.h>
<spawn.h>
<strings.h>
<stropts.h>
<sys/ipc.h>
<sys/mman.h>
<sys/msg.h>
<sys/resource.h>
<sys/select.h>
<sys/sem.h>
<sys/shm.h>
<sys/socket.h>
<sys/stat.h>
<sys/statvfs.h>
<sys/time.h>
<sys/timeb.h>
<sys/times.h>
<sys/uio.h>
<sys/un.h>
<sys/wait.h>
<syslog.h>
<termios.h>
<tgmath.h>
<time.h>
<trace.h>
<ucontext.h>
<ulimit.h>
<utime.h>
<utmpx.h>
<wchar.h>
<wctype.h>
<wordexp.h>
