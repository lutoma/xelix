#!/bin/env bash
set -e
set -x

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <destname>"
    return 1
fi

dest=$1

sudo modprobe nbd max_part=8

if [ -f "$dest" ]; then
	sudo qemu-nbd --connect=/dev/nbd0 "$dest"
	sudo mount /dev/nbd0p1 mnt
else
	qemu-img create -f qcow2 "$dest" 300M
	sudo qemu-nbd --connect=/dev/nbd0 "$dest"

	echo "/dev/nbd0p1 : start=2048, type=83" | sudo sfdisk /dev/nbd0
	sudo mkfs.ext2 /dev/nbd0p1
	sudo mount /dev/nbd0p1 mnt

	sudo grub-install /dev/nbd0 --boot-directory=mnt/boot --modules="normal part_msdos ext2 multiboot" --no-floppy --target=i386-pc
	cat <<EOF | sudo sponge mnt/boot/grub/grub.cfg
set root='hd0,msdos1'
set timeout=1

menuentry 'Xelix' {
	set gfxmode=720x480x24
	set gfxpayload=keep
	insmod vbe
	insmod gfxterm
	multiboot2 "(\$root)/boot/xelix.bin"
}
EOF

fi


for i in land/*; do
	echo "Building $i"
	make -C "$i"
	sudo make -C "$i" install
done

sudo umount mnt
sudo qemu-nbd --disconnect /dev/nbd0
