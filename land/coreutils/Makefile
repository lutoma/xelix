PACKAGE_VERSION := 8.30
PACKAGE_NAME := coreutils-$(PACKAGE_VERSION)
PACKAGE_URL := https://ftp.gnu.org/gnu/coreutils/$(PACKAGE_NAME).tar.xz


.PHONY: all
all: $(PACKAGE_NAME).tar.xz
	tar xf $(PACKAGE_NAME).tar.xz
	patch -p0 < ${PACKAGE_NAME}.patch
	cd ./$(PACKAGE_NAME) && ./configure --host=i786-pc-xelix --prefix=/usr --disable-threads --disable-acl --disable-xattr --disable-largefile
	patch -p0 < ${PACKAGE_NAME}-patch-two.patch
	echo 'int main() {return -1;}' > ./$(PACKAGE_NAME)/src/stat.c
	make -C $(PACKAGE_NAME)

.PHONY: install
install:
	cp $(PACKAGE_NAME)/src/date $(PACKAGE_NAME)/src/pwd $(PACKAGE_NAME)/src/b2sum $(PACKAGE_NAME)/src/basename $(PACKAGE_NAME)/src/echo $(PACKAGE_NAME)/src/env $(PACKAGE_NAME)/src/false $(PACKAGE_NAME)/src/head $(PACKAGE_NAME)/src/id $(PACKAGE_NAME)/src/md5sum $(PACKAGE_NAME)/src/mkdir $(PACKAGE_NAME)/src/printenv $(PACKAGE_NAME)/src/link $(PACKAGE_NAME)/src/printf $(PACKAGE_NAME)/src/sum $(PACKAGE_NAME)/src/tail $(PACKAGE_NAME)/src/true $(PACKAGE_NAME)/src/groups $(PACKAGE_NAME)/src/uname $(PACKAGE_NAME)/src/uniq $(PACKAGE_NAME)/src/whoami $(PACKAGE_NAME)/src/yes $(PACKAGE_NAME)/src/cat $(PACKAGE_NAME)/src/chmod $(PACKAGE_NAME)/src/chown $(PACKAGE_NAME)/src/sha1sum $(PACKAGE_NAME)/src/sha224sum $(PACKAGE_NAME)/src/sha256sum $(PACKAGE_NAME)/src/sha348sum $(PACKAGE_NAME)/src/sha512sum $(PACKAGE_NAME)/src/ls $(PACKAGE_NAME)/src/dir $(PACKAGE_NAME)/src/mv $(PACKAGE_NAME)/src/dirname $(PACKAGE_NAME)/src/rmdir $(PACKAGE_NAME)/src/rm $(PACKAGE_NAME)/src/unlink $(PACKAGE_NAME)/src/readlink $(PACKAGE_NAME)/src/du $(PACKAGE_NAME)/src/sync $(PACKAGE_NAME)/src/touch $(PACKAGE_NAME)/src/mktemp $(PACKAGE_NAME)/src/cp $(PACKAGE_NAME)/src/getlimits $(PACKAGE_NAME)/src/expand $(PACKAGE_NAME)/src/unexpand $(PACKAGE_NAME)/src/expr $(PACKAGE_NAME)/src/fold $(PACKAGE_NAME)/src/od $(PACKAGE_NAME)/src/paste $(PACKAGE_NAME)/src/tac $(PACKAGE_NAME)/src/tee $(PACKAGE_NAME)/src/test $(PACKAGE_NAME)/src/tr $(PACKAGE_NAME)/src/truncate ../../mnt/usr/bin

$(PACKAGE_NAME).tar.xz:
	wget --continue $(PACKAGE_URL) -O $(PACKAGE_NAME).tar.xz
