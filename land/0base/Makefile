.PHONY: all
all:
	true

.PHONY: install
install:
	install -d ../../mnt/home
	install -d ../../mnt/home/root
	install -d ../../mnt/dev
	install -d ../../mnt/etc
	install -d ../../mnt/sys
	install -d ../../mnt/tmp
	install -d ../../mnt/usr
	install -d ../../mnt/usr/bin
	install -d ../../mnt/usr/share

	echo 'root:x:0:0:root:/home/root:/usr/bin/dash' > ../../mnt/etc/passwd
	echo 'root:x:0:root' > ../../mnt/etc/group
	echo 'localhost.localdomain' > ../../mnt/etc/hostname
	cp motd ../../mnt/etc/
