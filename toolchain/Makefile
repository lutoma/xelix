TARGET=i786-pc-xelix
PREFIX=$(CURDIR)/local

BINUTILS_VERSION=2.31.1
BINUTILS_URL=http://ftp.gnu.org/gnu/binutils
BINUTILS_PACKAGE=binutils-${BINUTILS_VERSION}.tar.xz

GCC_VERSION=9.2.0
GCC_URL=http://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}
GCC_PACKAGE=gcc-${GCC_VERSION}.tar.xz

NEWLIB_VERSION=3.2.0
NEWLIB_URL=http://sourceware.org/pub/newlib
NEWLIB_PACKAGE=newlib-${NEWLIB_VERSION}.tar.gz

PATH:=$(PATH):$(CURDIR)/local/bin

.PHONY: all install clean binutils gcc newlib

all: binutils-all gcc-all newlib-all

clean: binutils-clean gcc-clean newlib-clean
	rm -rf build
	rm -rf download
	rm -rf ${PREFIX}

binutils-all: binutils-download binutils-patch binutils-make binutils-install
gcc-all: gcc-download gcc-patch gcc-make gcc-install
newlib-all: newlib-download newlib-patch newlib-make newlib-install

binutils-clean:
	rm -rf download/binutils-${BINUTILS_VERSION}/

binutils-download:
	mkdir -p download
	cd download && wget -c ${BINUTILS_URL}/${BINUTILS_PACKAGE}
	cd download && tar -xf ${BINUTILS_PACKAGE}

binutils-patch:
	patch -p0 -ddownload/binutils-${BINUTILS_VERSION}/ < ../land/binutils/binutils-${BINUTILS_VERSION}.patch
	cp ../land/binutils/xelix_i386.sh download/binutils-${BINUTILS_VERSION}/ld/emulparams/

binutils-make:
	mkdir -p build/binutils
	cd build/binutils && ../../download/binutils-${BINUTILS_VERSION}/configure --target=${TARGET} --prefix=${PREFIX} --disable-nls --disable-werror
	make -C build/binutils all

binutils-install:
	make -C build/binutils install

gcc-clean:
	rm -rf download/gcc-${GCC_VERSION}/

gcc-download:
	mkdir -p download
	cd download && wget -c ${GCC_URL}/${GCC_PACKAGE}
	cd download && tar -xf ${GCC_PACKAGE}

gcc-install:
	make -C build/gcc install-gcc
	make -C build/gcc install-target-libgcc
	make -C build/gcc install-target-libstdc++-v3
	ln -s i786-pc-xelix-gcc local/bin/i786-pc-xelix-cc

gcc-make:
	rm -r build/gcc
	mkdir -p build/gcc
	cd build/gcc && ../../download/gcc-${GCC_VERSION}/configure \
		--target=${TARGET} \
		--prefix=${PREFIX} \
		--disable-nls \
		--enable-languages=c,c++ \
		--with-headers=../../download/newlib-${NEWLIB_VERSION}/newlib/libc/include \
		--without-docdir

	make -C build/gcc all-gcc
	make -C build/gcc all-target-libgcc
	make -C build/gcc all-target-libstdc++-v3

gcc-patch:
	patch -p0 < patch/gcc-${GCC_VERSION}.patch
	cp content/gcc/xelix.h download/gcc-${GCC_VERSION}/gcc/config/

newlib-clean:
	rm -rf download/newlib-${NEWLIB_VERSION}/

newlib-download:
	mkdir -p download
	cd download && wget -c ${NEWLIB_URL}/${NEWLIB_PACKAGE}
	cd download && tar -xf ${NEWLIB_PACKAGE}

newlib-install:
	PATH="${PATH}:`pwd`/local/bin/" make -C build/newlib install
	cp build/newlib/i786-pc-xelix/newlib/libc/sys/xelix/crti.o build/newlib/i786-pc-xelix/newlib/libc/sys/xelix/crtn.o local/i786-pc-xelix/lib/

newlib-make:
	PATH="${PATH}:`pwd`/local/bin/" make -C build/newlib all

newlib-patch:
	-rm -r build/newlib/
	patch -p0 < patch/newlib-${NEWLIB_VERSION}.patch

	cp -r content/newlib download/newlib-${NEWLIB_VERSION}/newlib/libc/sys/xelix

	cd download/newlib-${NEWLIB_VERSION}/newlib/libc/sys/xelix/ && aclocal-1.11 -I ../../../ -I ../../../../
	cd download/newlib-${NEWLIB_VERSION}/newlib/libc/sys/xelix/ && autoconf-2.64
	cd download/newlib-${NEWLIB_VERSION}/newlib/libc/sys/xelix/ && automake-1.11 --cygnus Makefile

	cd download/newlib-${NEWLIB_VERSION}/newlib/libc/sys/ && aclocal-1.11 -I ../.. -I ../../..
	cd download/newlib-${NEWLIB_VERSION}/newlib/libc/sys/ && autoconf-2.64
	cd download/newlib-${NEWLIB_VERSION}/newlib/libc/sys/ && automake-1.11 --cygnus Makefile

	mkdir -p build/newlib
	cd build/newlib && PATH="${PATH}:`pwd`/local/bin/" ../../download/newlib-${NEWLIB_VERSION}/configure --target=${TARGET} --prefix=${PREFIX}
