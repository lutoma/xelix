PACKAGE_VERSION := 2.9.9
PACKAGE_NAME := libxml2-$(PACKAGE_VERSION)
PACKAGE_URL := ftp://xmlsoft.org/libxml2/$(PACKAGE_NAME).tar.gz

.PHONY: all
all: $(PACKAGE_NAME).tar.gz
	tar xf $(PACKAGE_NAME).tar.gz
	patch -p0 < ${PACKAGE_NAME}.patch
	cd ./$(PACKAGE_NAME) && CFLAGS="-I/home/lutoma/code/xelix/mnt/usr/include -L/home/lutoma/code/xelix/mnt/usr/lib" ./configure --host=i786-pc-xelix --prefix=/usr --without-threads --without-http --without-python
	make -C $(PACKAGE_NAME)

$(PACKAGE_NAME).tar.gz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.gz

.PHONY: install
install:
	make DESTDIR=$(abspath ../../mnt) -C $(PACKAGE_NAME) install
