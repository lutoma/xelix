PACKAGE_VERSION := 1.0.2
PACKAGE_NAME := bzip2-$(PACKAGE_VERSION)
PACKAGE_URL := ftp://sources.redhat.com/pub/bzip2/v102/$(PACKAGE_NAME).tar.gz

.PHONY: all
all: $(PACKAGE_NAME).tar.gz
	tar xf $(PACKAGE_NAME).tar.gz
	make -C $(PACKAGE_NAME) CC=i786-pc-xelix-gcc libbz2.a bzip2

$(PACKAGE_NAME).tar.gz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.gz

.PHONY: install
install:
	make PREFIX=$(abspath ../../mnt/usr) -C $(PACKAGE_NAME) install
