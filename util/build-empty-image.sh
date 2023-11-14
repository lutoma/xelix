#!/usr/bin/env bash
set -e
set -x

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <image_name>"
    exit 1
fi

dest=$1

qemu-img create -f raw "$dest" 10G
losetup -P /dev/loop5 "$dest"

cat <<EOF | sfdisk /dev/loop5
/dev/nbd0p1 : start=2048, size=1024000, type=83
/dev/nbd0p2 : start=1026048, type=83
EOF

# Docker uses tmpfs instead of devtmpfs for /dev, so doesn't pick up devices
# created after the container is started. This is an issue when we create new
# partitions below. Work around this by mounting separate devtmpfs.
mkdir /rdev || true
mount -t devtmpfs devs /rdev

mkfs.ext2 -b 4096 -O '^ext_attr,^dir_index' /rdev/loop5p1
mkfs.ext2 -b 4096 -O '^ext_attr,^dir_index' /rdev/loop5p2

mkdir -p mnt
mount /rdev/loop5p2 mnt
mkdir -p mnt/boot
mount /rdev/loop5p1 mnt/boot

grub-install /dev/loop5 --boot-directory=mnt/boot --modules="normal part_msdos ext2 multiboot" --no-floppy --target=i386-pc
mkdir -p mnt/boot/grub
cat <<EOF | sponge mnt/boot/grub/grub.cfg
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

menuentry 'Xelix (1920x1080)' {
	multiboot2 /xelix.bin root=/dev/ata1d1p2
	set gfxpayload=keep
}
 || true
menuentry 'Xelix (3840x2160)' {
	multiboot2 /xelix.bin root=/dev/ata1d1p2
	set gfxpayload=3840x2160x16
}

menuentry 'Xelix (Experimental GUI 1920x1080)' {
	multiboot2 /xelix.bin root=/dev/ata1d1p2 init_target=gui
	set gfxpayload=keep
}

menuentry 'Xelix (Experimental GUI 3840x2160 16-bit color)' {
	multiboot2 /xelix.bin root=/dev/ata1d1p2 init_target=gui
	set gfxpayload=3840x2160x16
}
EOF

mkdir -p mnt/var/lib/pacman/
umount mnt/boot
umount mnt
losetup -d /dev/loop5
