TARGET=i686-elf
PREFIX=$(CURDIR)/local

BINUTILS_VERSION=2.31
BINUTILS_URL=http://ftp.gnu.org/gnu/binutils
BINUTILS_PACKAGE=binutils-${BINUTILS_VERSION}.tar.xz

GCC_VERSION=8.1.0
GCC_URL=http://ftp.gnu.org/gnu/gcc/gcc-${GCC_VERSION}
GCC_PACKAGE=gcc-${GCC_VERSION}.tar.xz

PATH:=$(PATH):$(CURDIR)/local/bin

.PHONY: all install clean binutils gcc

all: binutils-all gcc-all

clean: binutils-clean gcc-clean
	@echo "Cleaning all"
	@rm -rf build
	@rm -rf download
	@rm -rf ${PREFIX}

download: binutils-download gcc-download

binutils-all: binutils-download binutils-make binutils-install

binutils-clean:
	@echo "Cleaning binutils"
	@rm -rf download/binutils-${BINUTILS_VERSION}/

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

gcc-all: gcc-download gcc-make gcc-install

gcc-clean:
	@echo "Cleaning gcc"
	@rm -rf download/gcc-${GCC_VERSION}/

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
#	@ln -s i586-pc-xelix-gcc local/bin/i586-pc-xelix-cc

gcc-make:
	@echo "Building gcc"
	@mkdir -p build/gcc
	-@find build/gcc -name config.cache -delete
	-@make -C build/gcc distclean
	@cd build/gcc && ../../download/gcc-${GCC_VERSION}/configure --target=${TARGET} --prefix=${PREFIX} --disable-nls --disable-shared --enable-languages=c --without-headers --without-docdir --disable-libssp
	@make -C build/gcc all-gcc
	@make -C build/gcc all-target-libgcc
