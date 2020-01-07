PACKAGE_VERSION := 1.1.0
PACKAGE_NAME := mpc-$(PACKAGE_VERSION)
PACKAGE_URL := https://ftp.gnu.org/gnu/mpc/$(PACKAGE_NAME).tar.gz

.PHONY: all
all: $(PACKAGE_NAME).tar.gz
	tar xf $(PACKAGE_NAME).tar.gz
	patch -p0 < ${PACKAGE_NAME}.patch

	cd ./$(PACKAGE_NAME) && ./configure --host=i786-pc-xelix --target=i786-pc-xelix --prefix=/usr --disable-shared \
		--with-gmp-include=$(abspath ../gmp/gmp-6.1.2) \
		--with-gmp-lib=$(abspath ../gmp/gmp-6.1.2/.libs) \
		--with-mpfr-include=$(abspath ../mpfr/mpfr-4.0.2/src) \
		--with-mpfr-lib=$(abspath ../mpfr/mpfr-4.0.2/src/.libs)

	make -C $(PACKAGE_NAME)

$(PACKAGE_NAME).tar.gz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.gz

.PHONY: install
install:
	make DESTDIR=$(abspath ../../mnt) -C $(PACKAGE_NAME) install
