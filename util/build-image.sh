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

qemu-img create -f qcow2 "$dest" 3G
sudo qemu-nbd --connect=/dev/nbd2 "$dest"

cat <<EOF | sudo sfdisk /dev/nbd2
/dev/nbd0p1 : start=2048, size=1024000, type=83
/dev/nbd0p2 : start=1026048, type=83
EOF

sudo mkfs.ext2 /dev/nbd2p1
sudo mkfs.ext2 /dev/nbd2p2

mkdir -p mnt
sudo mount /dev/nbd2p2 mnt
sudo mkdir -p mnt/boot
sudo mount /dev/nbd2p1 mnt/boot

sudo grub-install /dev/nbd2 --boot-directory=mnt/boot --modules="normal part_msdos ext2 multiboot" --no-floppy --target=i386-pc
cat <<EOF | sudo sponge mnt/boot/grub/grub.cfg
set root='hd0,msdos1'

if [ "x\${timeout}" != "x-1" ]; then
	if keystatus --shift; then
		set timeout=-1
	else
		set timeout=0
	fi
fi

menuentry 'Xelix' {
	multiboot2 /xelix.bin root=/dev/ide1p2
	set gfxpayload=1920x1080x32
}
EOF

sudo cp -r "$image_dir"/* mnt/
sudo cp xelix.bin mnt/boot/
sudo umount mnt/boot
sudo umount mnt
sudo qemu-nbd --disconnect /dev/nbd2
