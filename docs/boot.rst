Boot & startup
**************

Bootloader
==========

On x86, Xelix uses GRUB 2 as bootloader. In theory, any bootloader that understands the `Multiboot 2 <https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html>`_ standard should work. The kernel binary is a regular `ELF file <https://en.wikipedia.org/wiki/Executable_and_Linkable_Format>`_, and gets loaded into memory using the ELF program headers.

The Multiboot 2 standard also specifies a number of tags the kernel can leave in the beginning of the binary to request certain information or behaviour from the bootloader. These are located in :file:`src/boot/i386-boot.asm`. Xelix only sets the `Framebuffer request <https://www.gnu.org/software/grub/manual/multiboot2/multiboot.html#The-framebuffer-tag-of-Multiboot2-header>`_ tag.

Xelix uses a linker script for LD (:file:`src/boot/i386-linker.ld` for x86) to ensure it's always loaded at 0x100000. The linker script also ensures the multiboot headers are always at the beginning of the binary, and provides the code with `__kernel_start` and `__kernel_ènd` symbols.


Early startup
=============

After GRUB is done, it hands control to the entry function `_start` in :file:`src/boot/i386-boot.asm`. The assembly snippet stores the multiboot magic and header pointer left in CPU registers by GRUB, sets up the kernel stack and hands control to `main()` in :file:`src/boot/init.c`.
