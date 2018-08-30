set disassembly-flavor intel
shell make run QEMU_FLAGS+="-s -S -monitor vc -nographic" &
shell sleep 1
file xelix.bin
target remote localhost:1234
directory ./src
directory ./toolchain/
#add-symbol-file xelix.bin 0x10000
