TARGET=i786-pc-xelix
PREFIX=$(CURDIR)/local

BINUTILS_VERSION=2.31.1
BINUTILS_URL=http://ftp.gnu.org/gnu/binutils
BINUTILS_PACKAGE=binutils-${BINUTILS_VERSION}.tar.xz

GCC_VERSION=11.1.0
GCC_URL=http://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}
GCC_PACKAGE=gcc-${GCC_VERSION}.tar.xz

NEWLIB_VERSION=3.2.0
NEWLIB_URL=http://sourceware.org/pub/newlib
NEWLIB_PACKAGE=newlib-${NEWLIB_VERSION}.tar.gz

PATH:=$(PATH):$(CURDIR)/local/bin

.PHONY: toolchain install binutils-all gcc-all newlib-all

toolchain: binutils-all gcc-all newlib-make newlib-install

binutils-all: binutils-download binutils-patch binutils-make binutils-install
gcc-all: gcc-download newlib-download newlib-patch gcc-patch gcc-make gcc-install
newlib-all: newlib-download newlib-patch newlib-make newlib-install



binutils-download:
	mkdir -p download
	cd download && wget -c ${BINUTILS_URL}/${BINUTILS_PACKAGE}
	cd download && tar -xf ${BINUTILS_PACKAGE}

binutils-patch:
	patch -p0 -ddownload/binutils-${BINUTILS_VERSION}/ < ../land/binutils/binutils-${BINUTILS_VERSION}.patch
	find download/binutils-${BINUTILS_VERSION} -name "config.cache" -delete
	cp ../land/binutils/elf_i386_xelix.sh download/binutils-${BINUTILS_VERSION}/ld/emulparams/
	cd download/binutils-${BINUTILS_VERSION}/ld && aclocal
	cd download/binutils-${BINUTILS_VERSION}/ld && automake
	cd download/binutils-${BINUTILS_VERSION} && autoreconf-2.69

binutils-make:
	mkdir -p build/binutils
	cd build/binutils && ../../download/binutils-${BINUTILS_VERSION}/configure --target=${TARGET} --prefix=${PREFIX} --disable-nls --disable-werror
	make -C build/binutils all

binutils-install:
	make -C build/binutils install



gcc-download:
	mkdir -p download
	cd download && wget -c ${GCC_URL}/${GCC_PACKAGE}
	cd download && tar -xf ${GCC_PACKAGE}

gcc-patch:
	patch -p0 -ddownload/ < ../land/gcc/gcc-${GCC_VERSION}.patch
	cp ../land/gcc/xelix.h download/gcc-${GCC_VERSION}/gcc/config/

gcc-make:
	-rm -r build/gcc
	mkdir -p build/gcc
	cd build/gcc && ../../download/gcc-${GCC_VERSION}/configure \
		--target=${TARGET} \
		--prefix=${PREFIX} \
		--disable-nls \
		--enable-languages=c,c++ \
		--with-headers=../../download/newlib-${NEWLIB_VERSION}/newlib/libc/include \
		--without-docdir

	make -j 8 -C build/gcc all-gcc
	make -j 8 -C build/gcc all-target-libgcc

	# causes issues with newer GCC versions. Needs debugging
	#make -j 8 -C build/gcc all-target-libstdc++-v3

gcc-install:
	make -C build/gcc install-gcc
	make -C build/gcc install-target-libgcc

	#make -C build/gcc install-target-libstdc++-v3

	-ln -s i786-pc-xelix-gcc local/bin/i786-pc-xelix-cc



newlib-download:
	mkdir -p download
	cd download && wget -c ${NEWLIB_URL}/${NEWLIB_PACKAGE}
	cd download && tar -xf ${NEWLIB_PACKAGE}

newlib-install:
	PATH="${PATH}:`pwd`/local/bin/" make -C build/newlib install
	cp build/newlib/i786-pc-xelix/newlib/libc/sys/xelix/crti.o build/newlib/i786-pc-xelix/newlib/libc/sys/xelix/crtn.o local/i786-pc-xelix/lib/

	# Drop sys-include directory to avoid conflicts with regular includes
	rm -r local/i786-pc-xelix/sys-include


newlib-make:
	PATH="${PATH}:`pwd`/local/bin/" make -C build/newlib all

newlib-patch:
	-rm -r build/newlib/
	patch -p0 -ddownload/ < ../land/newlib/newlib-${NEWLIB_VERSION}.patch

	cp -r ../land/newlib/xelix download/newlib-${NEWLIB_VERSION}/newlib/libc/sys/

	cd download/newlib-${NEWLIB_VERSION}/newlib/libc/sys/xelix/ && aclocal-1.11 -I ../../../ -I ../../../../
	cd download/newlib-${NEWLIB_VERSION}/newlib/libc/sys/xelix/ && autoconf-2.69
	cd download/newlib-${NEWLIB_VERSION}/newlib/libc/sys/xelix/ && automake-1.11 --cygnus Makefile

	cd download/newlib-${NEWLIB_VERSION}/newlib/libc/sys/ && aclocal-1.11 -I ../.. -I ../../..
	cd download/newlib-${NEWLIB_VERSION}/newlib/libc/sys/ && autoconf-2.69
	cd download/newlib-${NEWLIB_VERSION}/newlib/libc/sys/ && automake-1.11 --cygnus Makefile

	mkdir -p build/newlib
	cd build/newlib && PATH="${PATH}:`pwd`/local/bin/" ../../download/newlib-${NEWLIB_VERSION}/configure --target=${TARGET} --prefix=${PREFIX}
