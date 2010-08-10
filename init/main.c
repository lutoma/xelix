#include <common/multiboot.h>
#include <common/generic.h>
#include <devices/display/interface.h>
#include <devices/cpu/interface.h>
#include <devices/keyboard/interface.h>
#include <memory/interface.h>
#include <interrupts/interface.h>
#include <devices/pit/interface.h>
#include <memory/kmalloc.h>
#include <filesystems/interface.h>
#include <filesystems/memfs/interface.h>

void checkIntLenghts();
void readInitrd(uint32 initrd_location);


void readInitrd(uint32 initrd_location)
{
	// Initialise the initial ramdisk, and set it as the filesystem root.
	fsRoot = memfs_init(initrd_location);

	// list the contents of /
	int i = 0;
	struct dirent *node = 0;
	while ( (node = readdirFs(fsRoot, i)) != 0)
	{
		print("Found file ");
		print(node->name);
		fsNode_t *fsnode = finddirFs(fsRoot, node->name);
		if ((fsnode->flags&0x7) == FS_DIRECTORY)
			print("\n	 (directory)\n");
		else
		{
			char* ext = substr(node->name, strlen(node->name) -4, 4);
			if(strcmp(ext, ".bin"))
			{
				print("\n	  contents: \"");
				char buf[256];
				uint32 sz = readFs(fsnode, 0, 256, buf);
				int j;
				for (j = 0; j < sz; j++)
					if(j < fsnode->length -1)
						display_printChar(buf[j]);
				print("\"");
			} else {
				print("\n	 Not showing contents of binary file");
			}
			print("\n");
		}
		i++;
	}
}

void kmain(multibootHeader_t *mboot_ptr)
{
	// check that our initrd was loaded by the bootloader and determine the addresses.
	ASSERT(mboot_ptr->mods_count > 0);
	uint32 initrd_location = *((uint32*)mboot_ptr->mods_addr);
	uint32 initrd_end = *(uint32*)(mboot_ptr->mods_addr+4);
	// Don't trample our module with placement accesses, please!
	kmalloc_init(initrd_end);

	log_init();
	display_init();
	
	display_setColor(0x0f);
	print("\n");
	print("                                   Xelix\n");
	print("\n");
	display_setColor(0x07);
	
	ASSERT(mboot_ptr->mods_count > 0); // If mods_count < 1, no initrd is loaded -> error.
	
	
	log("Initialized Display.\n");
	checkIntLenghts();
	memory_init_preprotected();
	log("Initialized preprotected memory\n");
	cpu_init();
	log("Initialized CPU\n");
	interrupts_init();
	log("Initialized interrupts\n");
	memory_init_postprotected();
	log("Initialized postprotected memory\n");
	pit_init(50); //50Hz
	log("Initialized PIT (programmable interrupt timer)\n");
	keyboard_init();
	log("Initialized keyboard\n");
	
	display_setColor(0x0f);
	log("Xelix is up.\n");
	display_setColor(0x07);
	
	log("Reading Initrd...\n");
	readInitrd(initrd_location);
	print("finished listing files\n");

	
	
	display_printHex(sizeof(size_t));
	
	
	while(1)

	{
		
	}
}


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
