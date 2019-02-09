PACKAGE_VERSION := 1.7.0
PACKAGE_NAME := picotcp-$(PACKAGE_VERSION)
PACKAGE_URL := https://github.com/tass-belgium/picotcp/archive/v$(PACKAGE_VERSION).tar.gz

.PHONY: all
all: $(PACKAGE_NAME).tar.gz
	tar xf $(PACKAGE_NAME).tar.gz
	cp pico_xelix.h $(PACKAGE_NAME)/include/arch
	patch -p0 < ${PACKAGE_NAME}.patch
	make PLATFORM_CFLAGS="-ffreestanding -I$(abspath ../../src) -I$(abspath ../../src/lib) -include $(abspath ../../src/lib/generic.h)" CROSS_COMPILE=i786-pc-xelix- -C $(PACKAGE_NAME)

$(PACKAGE_NAME).tar.gz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.gz
