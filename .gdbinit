set arch i386:x86-64
shell make run QEMU_FLAGS+="-s -S -monitor vc" &
shell sleep 1
file xelix.bin
target remote localhost:1234
directory ./src
directory ./toolchain/
#add-symbol-file xelix.bin 0x10000
