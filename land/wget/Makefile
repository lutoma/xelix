PACKAGE_VERSION := 1.20.3
PACKAGE_NAME := wget-$(PACKAGE_VERSION)
PACKAGE_URL := https://ftp.gnu.org/gnu/wget/$(PACKAGE_NAME).tar.gz

.PHONY: all
all: $(PACKAGE_NAME).tar.gz
	tar xf $(PACKAGE_NAME).tar.gz
	patch -p0 < ${PACKAGE_NAME}.patch
	cd $(PACKAGE_NAME) && CFLAGS="-I$(abspath ../../mnt/usr/include) -L$(abspath ../../mnt/usr/lib)" ./configure \
		--host=i786-pc-xelix \
		--prefix=/usr \
		--sysconfdir=/etc \
		--with-ssl=openssl \
		--disable-threads \
		--without-libpsl

	patch -p0 < ${PACKAGE_NAME}-two.patch
	make -C $(PACKAGE_NAME)

$(PACKAGE_NAME).tar.gz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.gz

.PHONY: install
install:
	make DESTDIR=$(abspath ../../mnt) -C $(PACKAGE_NAME) install
