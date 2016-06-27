shell make runqemu QEMU_FLAGS+="-s -S -monitor vc -hda xsfs.img" &
shell sleep 1
file xelix.bin
target remote localhost:1234
directory ./src
directory ./toolchain/
#add-symbol-file xelix.bin 0x10000
