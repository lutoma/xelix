PATH += :../../toolchain/local/bin/
CC := i786-pc-xelix-gcc
CFLAGS += -std=gnu18 -O3 -g -D_GNU_SOURCE

TARGETS=basictest init ps uptime free login dmesg

all: $(TARGETS)

.PHONY: clean
clean:
	rm -f $(TARGETS) util.o libutil.a basictest.o

%: %.asm
	nasm -g -f elf -o $@.o $<
	$(CC) -nostdlib -o $@ $@.o

%: %.c libutil.a
	$(CC) $(CFLAGS) -o $@ $@.c -L. -lutil

libutil.a: util.c argparse.c
	$(CC) $(CFLAGS) -c -o util.o util.c
	$(CC) $(CFLAGS) -c -o argparse.o argparse.c
	ar rcs $@ util.o argparse.o

.PHONY: install
install:
	install -d ../../mnt/usr/bin/
	install -D $(TARGETS) ../../mnt/usr/bin/
