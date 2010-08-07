#include <common/multiboot.h>
#include <common/generic.h>
#include <devices/display/interface.h>
#include <devices/cpu/interface.h>
#include <devices/keyboard/interface.h>
#include <memory/segmentation/gdt.h>
#include <interrupts/idt.h>
#include <interrupts/irq.h>
#include <devices/pit/interface.h>
#include <init/debugconsole.h>
#include <memory/kmalloc.h>
#include <common/bitmap.h>
#include <filesystems/interface.h>
#include <filesystems/initrd/interface.h>

void checkIntLenghts();

void checkIntLenghts()
{
	log("Checking length of uint8... ");
	ASSERT(sizeof(uint8) == 1);
	log("Right\n");
	
	log("Checking length of uint16... ");
	ASSERT(sizeof(uint16) == 2);
	log("Right\n");
	
	log("Checking length of uint32... ");
	ASSERT(sizeof(uint32) == 4);
	log("Right\n");
}

void kmain(struct multiboot *mboot_ptr)
{
	log_init();
	display_init();
	
	display_setColor(0x0f);
	print("                                               \n");
	print("                                     decore    \n");
	print("                                               \n");
	display_setColor(0x07);
	
	
	
	log("Initialized Display.\n");
	initAcpi();
	log("Initialized ACPI (Advanced Configuration and Power Interface)\n");
	checkIntLenghts();
	cpu_init();
	log("Initialized CPU\n");
	gdt_init();
	log("Initialized GDT (global descriptor table)\n");
	idt_init();
	log("Initialized IDT (interrupt descriptor table)\n");
	pit_init(50); //50Hz
	log("Initialized PIT (programmable interrupt timer)\n");
	keyboard_init();
	log("Initialized keyboard\n");
	//memory_init();
	//log("Initialized memory.\n");
	
	//uint32 *ptr = (uint32*)0xA0000000;
	//uint32 do_page_fault = *ptr;
	
   display_printDec(mboot_ptr->mods_count);
	 ASSERT(mboot_ptr->mods_count > 0);
   uint32 initrd_location = *((uint32*)mboot_ptr->mods_addr);
   uint32 initrd_end = *(uint32*)(mboot_ptr->mods_addr+4);
   kmalloc_init(initrd_end);

   // Initialise the initial ramdisk, and set it as the filesystem root.
   fs_root = initialise_initrd(initrd_location);

	log("Decore is up.\n");

	// list the contents of /
	int i = 0;
	struct dirent *node = 0;
	while ( (node = readdir_fs(fs_root, i)) != 0)
	{
		print("Found file ");
		print(node->name);
		fs_node_t *fsnode = finddir_fs(fs_root, node->name);

		if ((fsnode->flags&0x7) == FS_DIRECTORY)
			print("\n\t(directory)\n");
		else
		{
			print("\n\t contents: \"");
			char buf[256];
			uint32 sz = read_fs(fsnode, 0, 256, buf);
			int j;
			for (j = 0; j < sz; j++)
				print(buf[j]);

			print("\"\n");
		}
		i++;
	}
	//debugconsole_init();
	while(1)
	{
		
	}
}
