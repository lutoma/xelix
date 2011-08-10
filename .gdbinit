shell qemu -d cpu_reset -monitor stdio -ctrl-grab -net nic -net user -kernel xelix.bin -s -S
target remote localhost:1234
