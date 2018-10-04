PATH += :../../toolchain/local/bin/
CC := i786-pc-xelix-gcc
CFLAGS += -std=gnu18 -O3 -g -D_GNU_SOURCE

TARGETS=basictest xinit xshell ps uptime free

all: $(TARGETS)

.PHONY: clean
clean:
	rm -f $(TARGETS) util.o libutil.a basictest.o

%: %.asm
	nasm -g -f elf -o $@.o $<
	$(CC) -nostdlib -o $@ $@.o

%: %.c libutil.a
	$(CC) $(CFLAGS) -o $@ $@.c -L. -lutil

libutil.a: util.c
	$(CC) $(CFLAGS) -c -o util.o $<
	ar rcs $@ util.o

.PHONY: install
install:
	install -d ../../mnt/usr/bin/
	install -D $(TARGETS) ../../mnt/usr/bin/
