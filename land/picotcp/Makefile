PACKAGE_VERSION := 1.7.0
PACKAGE_NAME := picotcp-$(PACKAGE_VERSION)
PACKAGE_URL := https://github.com/tass-belgium/picotcp/archive/v$(PACKAGE_VERSION).tar.gz


$(PACKAGE_NAME).tar.gz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.gz

.PHONY: all
all: $(PACKAGE_NAME).tar.gz
	tar xf $(PACKAGE_NAME).tar.gz
	cp pico_xelix.h $(PACKAGE_NAME)/include/arch
	cp devxelix.mk $(PACKAGE_NAME)/rules
	cp pico_dev_xelix.c pico_dev_xelix.h $(PACKAGE_NAME)/modules
	patch -p0 < ${PACKAGE_NAME}.patch
	make CROSS_COMPILE=i786-pc-xelix- -C $(PACKAGE_NAME)

.PHONY: install
install:
	make CROSS_COMPILE=i786-pc-xelix- -C $(PACKAGE_NAME) install
