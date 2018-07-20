CC=../toolchain/userspace/local/bin/i586-pc-xelix-gcc
LD=../toolchain/userspace/local/bin/i586-pc-xelix-ld
CFLAGS="-std=gnu99"

TARGETS=xinit xshell cat ls pwd cd

.PHONY: all
all: $(TARGETS)

.PHONY: clean
clean:
	rm -f $(TARGETS)

.PHONY: image
image: $(TARGETS)
	../util/build-xsfs.sh  ../xsfs.img $(TARGETS)
