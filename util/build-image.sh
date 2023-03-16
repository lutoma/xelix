#!/bin/env bash
set -e
set -x

if [ "$#" -ne 2 ]; then
    echo "Usage: $0 <image_dir> <image_name>"
    exit 1
fi

image_dir=$1
dest=$2

sudo modprobe nbd max_part=8

qemu-img create -f raw "$dest" 10G
sudo losetup -P /dev/loop2 "$dest"

cat <<EOF | sudo sfdisk /dev/loop2
/dev/nbd0p1 : start=2048, size=1024000, type=83
/dev/nbd0p2 : start=1026048, type=83
EOF

sudo mkfs.ext2 /dev/loop2p1 -b 4096 -O '^ext_attr,^dir_index'
sudo mkfs.ext2 /dev/loop2p2 -b 4096 -O '^ext_attr,^dir_index'

mkdir -p mnt
sudo mount /dev/loop2p2 mnt
sudo mkdir -p mnt/boot
sudo mount /dev/loop2p1 mnt/boot

sudo grub-install /dev/loop2 --boot-directory=mnt/boot --modules="normal part_msdos ext2 multiboot" --no-floppy --target=i386-pc
cat <<EOF | sudo sponge mnt/boot/grub/grub.cfg
set root='hd0,msdos1'
set gfxmode=1920x1080x32
loadfont unicode
terminal_output gfxterm

if [ "x${timeout}" != "x-1" ]; then
	if keystatus --shift; then
		set timeout=-1
	else
		set timeout=5
	fi
fi

menuentry 'Xelix' {
	multiboot2 /xelix.bin root=/dev/ide1p2
	set gfxpayload=keep
}

menuentry 'Xelix (Text mode)' {
	multiboot2 /xelix.bin root=/dev/ide1p2 init=/usr/bin/login
	set gfxpayload=keep
}
EOF

sudo cp -r "$image_dir"/* mnt/
sudo cp xelix.bin mnt/boot/
sudo umount mnt/boot
sudo umount mnt
sudo losetup -d /dev/loop2
