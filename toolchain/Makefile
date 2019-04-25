TARGET=i786-pc-xelix
PREFIX=$(CURDIR)/local

BINUTILS_VERSION=2.31.1
BINUTILS_URL=http://ftp.gnu.org/gnu/binutils
BINUTILS_PACKAGE=binutils-${BINUTILS_VERSION}.tar.xz

GCC_VERSION=8.3.0
GCC_URL=http://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}
GCC_PACKAGE=gcc-${GCC_VERSION}.tar.xz

NEWLIB_VERSION=3.1.0.20181231
NEWLIB_URL=ftp://sources.redhat.com/pub/newlib
NEWLIB_PACKAGE=newlib-${NEWLIB_VERSION}.tar.gz

PATH:=$(PATH):$(CURDIR)/local/bin

.PHONY: all install clean binutils gcc newlib

all: binutils-all gcc-all newlib-all

clean: binutils-clean gcc-clean newlib-clean
	@echo "Cleaning all"
	@rm -rf build
	@rm -rf download
	@rm -rf ${PREFIX}

download: binutils-download gcc-download newlib-download

binutils-all: binutils-download binutils-patch binutils-make binutils-install

binutils-clean:
	@echo "Cleaning binutils"
	@rm -rf download/binutils-${BINUTILS_VERSION}/

binutils-diff:
	@mv download/binutils-${BINUTILS_VERSION} download/binutils-${BINUTILS_VERSION}-xelix
	@cd download && tar -xf ${BINUTILS_PACKAGE}
	@diff -rupN download/binutils-${BINUTILS_VERSION}/ download/binutils-${BINUTILS_VERSION}-xelix/ > patch/binutils-${BINUTILS_VERSION}.patch || :

binutils-download:
	@mkdir -p download
	@echo "Downloading binutils"
	@cd download && wget -c ${BINUTILS_URL}/${BINUTILS_PACKAGE}
	@echo "Unpacking binutils"
	@cd download && tar -xf ${BINUTILS_PACKAGE}

binutils-install:
	@echo "Installing binutils"
	@make -C build/binutils install

binutils-make:
	@echo "Building binutils"
	@mkdir -p build/binutils
	@cd build/binutils && ../../download/binutils-${BINUTILS_VERSION}/configure --target=${TARGET} --prefix=${PREFIX} --disable-nls --disable-werror
	@make -C build/binutils all

binutils-patch:
	@echo "Patching binutils"
	@patch -p0 < patch/binutils-${BINUTILS_VERSION}.patch
	@cp content/binutils/xelix_i386.sh download/binutils-${BINUTILS_VERSION}/ld/emulparams/
	@cp content/binutils/xelix_arm.sh download/binutils-${BINUTILS_VERSION}/ld/emulparams/

gcc-all: gcc-download gcc-patch gcc-make gcc-install

gcc-clean:
	@echo "Cleaning gcc"
	@rm -rf download/gcc-${GCC_VERSION}/

gcc-diff:
	@mv download/gcc-${GCC_VERSION} download/gcc-${GCC_VERSION}-xelix
	@cd download && tar -xf ${GCC_PACKAGE}
	@diff -rupN download/gcc-${GCC_VERSION}/ download/gcc-${GCC_VERSION}-xelix/ > patch/gcc-${GCC_VERSION}.patch || :

gcc-download:
	@mkdir -p download
	@echo "Downloading gcc"
	@cd download && wget -c ${GCC_URL}/${GCC_PACKAGE}
	@echo "Unpacking gcc"
	@cd download && tar -xf ${GCC_PACKAGE}

gcc-install:
	@echo "Installing gcc"
	@make -C build/gcc install-gcc
	@make -C build/gcc install-target-libgcc
	@make -C build/gcc install-target-libstdc++-v3
	@ln -s arm-none-xelix-gcc local/bin/arm-none-xelix-cc

gcc-make:
	@echo "Building gcc"
	@rm -r build/gcc
	@mkdir -p build/gcc
	@cd build/gcc && ../../download/gcc-${GCC_VERSION}/configure \
		--target=${TARGET} \
		--prefix=${PREFIX} \
		--disable-nls \
		--enable-languages=c,c++ \
		--with-headers=../../download/newlib-3.1.0.20181231/newlib/libc/include \
		--without-docdir

	@make -C build/gcc all-gcc
	@make -C build/gcc all-target-libgcc
	@make -C build/gcc all-target-libstdc++-v3

gcc-patch:
	@echo "Patching gcc"
	@patch -p0 < patch/gcc-${GCC_VERSION}.patch
	@cp content/gcc/xelix.h download/gcc-${GCC_VERSION}/gcc/config/

newlib-all: newlib-download newlib-copy newlib-reconfig newlib-make newlib-install

newlib-clean:
	@echo "Cleaning newlib"
	@rm -rf download/newlib-${NEWLIB_VERSION}/

newlib-diff:
	@mv download/newlib-${NEWLIB_VERSION} download/newlib-${NEWLIB_VERSION}-xelix
	@cd download && tar -xf ${NEWLIB_PACKAGE}
	@diff -rupN download/newlib-${NEWLIB_VERSION}/ download/newlib-${NEWLIB_VERSION}-xelix/ > patch/newlib-${NEWLIB_VERSION}.patch || :

newlib-download:
	@mkdir -p download
	@echo "Downloading newlib"
	@cd download && wget -c ${NEWLIB_URL}/${NEWLIB_PACKAGE}
	@echo "Unpacking newlib"
	@cd download && tar -xf ${NEWLIB_PACKAGE}

newlib-install:
	@echo "Installing newlib"
	@PATH="${PATH}:`pwd`/local/bin/" make -C build/newlib install
	@cp build/newlib/i786-pc-xelix/newlib/libc/sys/xelix/crti.o local/i786-pc-xelix/lib/
	@cp build/newlib/i786-pc-xelix/newlib/libc/sys/xelix/crtn.o local/i786-pc-xelix/lib/

newlib-make:
	@echo "Building newlib"
	@PATH="${PATH}:`pwd`/local/bin/" make -C build/newlib all

newlib-copy:
	@mkdir -p download/newlib-${NEWLIB_VERSION}/newlib/libc/sys/xelix
	@cp -R content/newlib/* download/newlib-${NEWLIB_VERSION}/newlib/libc/sys/xelix/

newlib-reconfig:
	@echo "Patching newlib autoconf files"
	@-rm -r build/newlib/
	@-patch -p0 < patch/newlib-${NEWLIB_VERSION}.patch

	@cd download/newlib-${NEWLIB_VERSION}/newlib/libc/sys/xelix/ && aclocal-1.11 -I ../../../ -I ../../../../
	@cd download/newlib-${NEWLIB_VERSION}/newlib/libc/sys/xelix/ && autoconf-2.64
	@cd download/newlib-${NEWLIB_VERSION}/newlib/libc/sys/xelix/ && automake-1.11 --cygnus Makefile

	@cd download/newlib-${NEWLIB_VERSION}/newlib/libc/sys/ && aclocal-1.11 -I ../.. -I ../../..
	@cd download/newlib-${NEWLIB_VERSION}/newlib/libc/sys/ && autoconf-2.64
	@cd download/newlib-${NEWLIB_VERSION}/newlib/libc/sys/ && automake-1.11 --cygnus Makefile

	@mkdir -p build/newlib
	@cd build/newlib && PATH="${PATH}:`pwd`/local/bin/" ../../download/newlib-${NEWLIB_VERSION}/configure --target=${TARGET} --prefix=${PREFIX}
