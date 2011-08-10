shell qemu -d cpu_reset -ctrl-grab -net nic -net user -kernel xelix.bin -s -S
file xelix.bin
target remote localhost:1234
