CC := i786-pc-xelix-gcc
LD := i786-pc-xelix-ld
CFLAGS += -std=gnu18 -O3 -ggdb -D_GNU_SOURCE
DESTDIR ?= ../../../mnt

TARGETS=basictest ps uptime free login dmesg su play strace host telnetd mount umount gfxterm png xelix-loader

.PHONY: all
all: $(TARGETS) init xelix-loader

.PHONY: clean
clean:
	rm -f $(TARGETS) util.o libutil.a init basictest.o xelix-loader ld-xelix.o ld-xelix-i386.o

init: init.c ini.c libutil.a
	$(CC) $(CFLAGS) -fPIC -o $@ init.c ini.c -L. -lutil

play: play.c
	$(CC) $(CFLAGS) -fPIC -o $@ play.c -L. -L/usr/lib/gcc/i786-pc-xelix/13.1.0 -lFLAC -lutil -lm

gfxterm: gfxterm.c tmt.c tmt.h
	$(CC) $(CFLAGS) -fPIC -o $@ tmt.c gfxterm.c -lxelixgfx -lfreetype -lpng -lz -lm -lbz2

png: png.c
	$(CC) $(CFLAGS) -fPIC -o $@ png.c -lxelixgfx -lpng -lz -lm

xelix-loader: xelix-loader.c xelix-loader-i386.asm xelix-loader.ld
	$(CC) $(CFLAGS) -o xelix-loader.o -c xelix-loader.c
	nasm -g -f elf xelix-loader-i386.asm -o xelix-loader-i386.o
	$(CC) $(CFLAGS) -o $@ xelix-loader.o xelix-loader-i386.o -static -fno-pie -T xelix-loader.ld

libutil.a: util.c argparse.c
	$(CC) $(CFLAGS) -fPIC -c -o util.o util.c
	$(CC) $(CFLAGS) -fPIC -c -o argparse.o argparse.c
	ar rcs $@ util.o argparse.o

%: %.asm
	nasm -g -f elf -o $@.o $<
	$(LD) -nostdlib -o $@ $@.o

%: %.c libutil.a
	$(CC) $(CFLAGS) -fPIC -o $@ $@.c -L. -lutil

.PHONY: install
install:
	install -d $(DESTDIR)/usr/bin/
	install -d $(DESTDIR)/usr/lib/
	install -d $(DESTDIR)/usr/libexec/system/
	install -D $(TARGETS) $(DESTDIR)/usr/bin/
	install -D init xelix-loader $(DESTDIR)/usr/libexec/system
	chmod u+s $(DESTDIR)/usr/bin/su
