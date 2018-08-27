PACKAGE_VERSION := 1.29.2
PACKAGE_NAME := busybox-$(PACKAGE_VERSION)
PACKAGE_URL := https://busybox.net/downloads/$(PACKAGE_NAME).tar.bz2


$(PACKAGE_NAME).tar.bz2:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.bz2

.PHONY: all
all: $(PACKAGE_NAME).tar.bz2
	tar xf $(PACKAGE_NAME).tar.bz2
	cp config $(PACKAGE_NAME)/.config
	patch -p0 < ${PACKAGE_NAME}.patch
	make CROSS_COMPILE=i786-pc-xelix- -C $(PACKAGE_NAME)

.PHONY: install
install:
	make CROSS_COMPILE=i786-pc-xelix- -C $(PACKAGE_NAME) install
