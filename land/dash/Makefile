PACKAGE_VERSION := 0.5.10.2
PACKAGE_NAME := dash-$(PACKAGE_VERSION)
PACKAGE_URL := http://gondor.apana.org.au/~herbert/dash/files/$(PACKAGE_NAME).tar.gz

$(PACKAGE_NAME).tar.gz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.gz

.PHONY: all
all: $(PACKAGE_NAME).tar.gz
	tar xf $(PACKAGE_NAME).tar.gz
	cd ./$(PACKAGE_NAME) && ./configure --host=i786-pc-xelix --prefix=/
	patch -p0 < ${PACKAGE_NAME}.patch
	make -C $(PACKAGE_NAME)

.PHONY: install
install:
	make DESTDIR=$(abspath ../../mnt) -C $(PACKAGE_NAME) install
