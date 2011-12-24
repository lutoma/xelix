shell make runqemu QEMU_FLAGS+="-s -S -monitor vc" &
shell sleep 1
file xelix.bin
target remote localhost:1234
