PACKAGE_VERSION := 20191231-3.1
PACKAGE_NAME := libedit-$(PACKAGE_VERSION)
PACKAGE_URL := https://thrysoee.dk/editline/$(PACKAGE_NAME).tar.gz

.PHONY: all
all: $(PACKAGE_NAME).tar.gz
	tar xf $(PACKAGE_NAME).tar.gz
	patch -p0 < ${PACKAGE_NAME}.patch
	cd $(PACKAGE_NAME) && CFLAGS="-D__STDC_ISO_10646__" ./configure --prefix=/usr --host=i786-pc-xelix --disable-shared --disable-widec
	make -C $(PACKAGE_NAME)

$(PACKAGE_NAME).tar.gz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.gz

.PHONY: install
install:
	make DESTDIR=$(abspath ../../mnt) -C $(PACKAGE_NAME) install
