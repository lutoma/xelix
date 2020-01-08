PACKAGE_VERSION := 9.2.0
PACKAGE_NAME := gcc-$(PACKAGE_VERSION)
PACKAGE_URL := https://ftp.gnu.org/gnu/gcc/gcc-9.2.0/$(PACKAGE_NAME).tar.gz

.PHONY: all
all: $(PACKAGE_NAME).tar.gz
	tar xf $(PACKAGE_NAME).tar.gz
	patch -p0 < ${PACKAGE_NAME}.patch
	cp xelix.h $(PACKAGE_NAME)/gcc/config/

	-rm -r build
	mkdir build
	cd build && BOOT_CFLAGS="-D_GLIBCXX_USE_C99_LONG_LONG_DYNAMIC=0 -D_GLIBCXX_USE_C99_STDLIB=0" CPPFLAGS="-D_GLIBCXX_USE_C99_LONG_LONG_DYNAMIC=0 -D_GLIBCXX_USE_C99_STDLIB=0" CFLAGS="-D_GLIBCXX_USE_C99_LONG_LONG_DYNAMIC=0 -D_GLIBCXX_USE_C99_STDLIB=0" ../${PACKAGE_NAME}/configure \
		--host=i786-pc-xelix \
		--target=i786-pc-xelix \
		--disable-shared \
		--enable-static \
		--prefix=/usr \
		--libdir=/usr/lib \
		--libexecdir=/usr/lib \
		--mandir=/usr/share/man \
		--infodir=/usr/share/info \
		--with-bugurl=https://github.com/lutoma/xelix \
		--enable-languages=c,c++ \
		--with-sysroot=$(abspath ../../mnt) \
		--with-gmp-include=$(abspath ../gmp/gmp-6.1.2) \
		--with-gmp-lib=$(abspath ../gmp/gmp-6.1.2/.libs) \
		--with-mpfr-include=$(abspath ../mpfr/mpfr-4.0.2/src) \
		--with-mpfr-lib=$(abspath ../mpfr/mpfr-4.0.2/src/.libs) \
		--with-mpc-include=$(abspath ../mpc/mpc-1.1.0/src) \
		--with-mpc-lib=$(abspath ../mpc/mpc-1.1.0/src/.libs)

	BOOT_CFLAGS="-D_GLIBCXX_USE_C99_LONG_LONG_DYNAMIC=0 -D_GLIBCXX_USE_C99_STDLIB=0" CPPFLAGS="-D_GLIBCXX_USE_C99_LONG_LONG_DYNAMIC=0 -D_GLIBCXX_USE_C99_STDLIB=0" CFLAGS="-D_GLIBCXX_USE_C99_LONG_LONG_DYNAMIC=0 -D_GLIBCXX_USE_C99_STDLIB=0" make -C build all-gcc
	make -C build all-target-libgcc
	#make -C build all-target-libstdc++-v3

$(PACKAGE_NAME).tar.gz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.gz

.PHONY: install
install:
	make DESTDIR=$(abspath ../../mnt) -C build install-gcc
	make DESTDIR=$(abspath ../../mnt) -C build install-target-libgcc
	#make DESTDIR=$(abspath ../../mnt) -C build install-target-libstdc++-v3

