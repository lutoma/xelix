## Process this file with automake to generate Makefile.in

AUTOMAKE_OPTIONS = cygnus

INCLUDES = $(NEWLIB_CFLAGS) $(CROSS_CFLAGS) $(TARGET_CFLAGS)

AM_CCASFLAGS = $(INCLUDES)

noinst_LIBRARIES = lib.a

if MAY_SUPPLY_SYSCALLS
extra_objs = $(lpfx)syscalls.o stubs.o inet_addr.o inet_ntoa.o getgrent.o mntent.o mntent_r.o getaddrinfo.o openpty.o pututline.o select.o
else
extra_objs =
endif

lib_a_SOURCES =
lib_a_LIBADD = $(extra_objs)
EXTRA_lib_a_SOURCES = crt0.c crti.s crtn.s syscalls.c stubs.c inet_addr.c inet_ntoa.c getgrent.c mntent.c mntent_r.c getaddrinfo.c openpty.c pututline.c select.c
lib_a_DEPENDENCIES = $(extra_objs)
lib_a_CCASFLAGS = $(AM_CCASFLAGS)
lib_a_CFLAGS = $(AM_CFLAGS)

if MAY_SUPPLY_SYSCALLS
all: crt0.o crti.o crtn.o
endif

ACLOCAL_AMFLAGS = -I ../../.. -I ../../../..
CONFIG_STATUS_DEPENDENCIES = $(newlib_basedir)/configure.host

install-data-local:
		$(mkinstalldirs) $(DESTDIR)$(tooldir)/include/arpa; \
		for i in $(srcdir)/include/arpa/*.h; do \
		  $(INSTALL_DATA) $$i $(DESTDIR)$(tooldir)/include/arpa/`basename $$i`; \
		done; \
		$(mkinstalldirs) $(DESTDIR)$(tooldir)/include/net; \
		for i in $(srcdir)/include/net/*.h; do \
		  $(INSTALL_DATA) $$i $(DESTDIR)$(tooldir)/include/net/`basename $$i`; \
		done; \
		$(mkinstalldirs) $(DESTDIR)$(tooldir)/include/netinet; \
		for i in $(srcdir)/include/netinet/*.h; do \
		  $(INSTALL_DATA) $$i $(DESTDIR)$(tooldir)/include/netinet/`basename $$i`; \
		done; \
		$(mkinstalldirs) $(DESTDIR)$(tooldir)/include; \
		for i in $(srcdir)/include/*.h; do \
		  $(INSTALL_DATA) $$i $(DESTDIR)$(tooldir)/include/`basename $$i`; \
		done;
