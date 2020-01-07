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
	cd build && ../${PACKAGE_NAME}/configure \
		--host=i786-pc-xelix \
		--target=i786-pc-xelix \
		--disable-shared \
		--enable-static \
		--disable-libcpp \
		--disable-libstdcxx \
		--disable-shared \
		--disable-multilib \
		--disable-decimal-float \
		--disable-threads \
		--disable-libatomic \
		--disable-libgomp \
		--disable-libitm \
		--disable-libquadmath \
		--disable-libsanitizer \
		--disable-libssp \
		--disable-libvtv \
		--disable-libcilkrts \
		--prefix=$(abspath ../../mnt) \
		--enable-languages=c \
		--with-sysroot=$(abspath ../../mnt) \
		--with-gmp-include=$(abspath ../gmp/gmp-6.1.2) \
		--with-gmp-lib=$(abspath ../gmp/gmp-6.1.2/.libs) \
		--with-mpfr-include=$(abspath ../mpfr/mpfr-4.0.2/src) \
		--with-mpfr-lib=$(abspath ../mpfr/mpfr-4.0.2/src/.libs) \
		--with-mpc-include=$(abspath ../mpc/mpc-1.1.0/src) \
		--with-mpc-lib=$(abspath ../mpc/mpc-1.1.0/src/.libs)

	make -C build
#	make -C build all-gcc
#	make -C build all-target-libgcc
#	make -C build all-target-libstdc++-v3


#	--with-headers=$(abspath ../../toolchain/download/newlib-3.2.0/newlib/libc/include) \

$(PACKAGE_NAME).tar.gz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.gz

.PHONY: install
install:
	make DESTDIR=$(abspath ../../mnt) -C $(PACKAGE_NAME) install
