PACKAGE_VERSION := 3.2.0
PACKAGE_NAME := newlib-$(PACKAGE_VERSION)
PACKAGE_URL := http://sourceware.org/pub/newlib/$(PACKAGE_NAME).tar.gz

.PHONY: all
all: $(PACKAGE_NAME).tar.gz
	tar xf $(PACKAGE_NAME).tar.gz
	patch -p0 < ${PACKAGE_NAME}.patch
	cp -r xelix ${PACKAGE_NAME}/newlib/libc/sys/

	cd ${PACKAGE_NAME}/newlib/libc/sys/xelix/ && aclocal-1.11 -I ../../../ -I ../../../../
	cd ${PACKAGE_NAME}/newlib/libc/sys/xelix/ && autoconf-2.64
	cd ${PACKAGE_NAME}/newlib/libc/sys/xelix/ && automake-1.11 --cygnus Makefile

	cd ${PACKAGE_NAME}/newlib/libc/sys/ && aclocal-1.11 -I ../.. -I ../../..
	cd ${PACKAGE_NAME}/newlib/libc/sys/ && autoconf-2.64
	cd ${PACKAGE_NAME}/newlib/libc/sys/ && automake-1.11 --cygnus Makefile

	-rm -r build
	mkdir build
	cd build && ../${PACKAGE_NAME}/configure \
		--target=i786-pc-xelix \
		--prefix=/usr

	make -C build

$(PACKAGE_NAME).tar.gz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.gz

.PHONY: install
install:
	# Work around autoconf issues
	make DESTDIR=$(abspath build/cache) -C build install
	cp -r build/cache/usr/i786-pc-xelix/* ../../mnt/usr/

	cp build/i786-pc-xelix/newlib/libc/sys/xelix/crti.o build/i786-pc-xelix/newlib/libc/sys/xelix/crtn.o ../../mnt/usr/lib/
