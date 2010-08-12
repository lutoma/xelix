#include <common/multiboot.h>
#include <common/generic.h>
#include <common/string.h>
#include <devices/display/interface.h>
#include <devices/cpu/interface.h>
#include <devices/keyboard/interface.h>
#include <memory/interface.h>
#include <interrupts/interface.h>
#include <devices/pit/interface.h>
#include <memory/kmalloc.h>
#include <filesystems/interface.h>
#include <filesystems/memfs/interface.h>
#include <devices/pit/interface.h>

#include <processes/process.h>

void checkIntLenghts();

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
				uint32 sz = readFs(fsnode, 0, 256, (uint8*) buf);
				int j;
				for (j = 0; j < sz; j++)
					if(j < fsnode->length -1)
					{
						char s[2];
						s[0] = buf[j];
						s[1] = '\0';
						print(s);
					}
				print("\"");
			} else {
				print("\n	 Not showing contents of binary file");
			}
			print("\n");
		}
		i++;
	}
}


void printAlphabet()
{
	uint32* a = 0x1000*1024*4+2;
	print("alphabet");
	*a = 3141;
	printDec(*a);
	while(1)
	{
	}
	char abc[] = "abcdefghijklmnopqrstuvwxyz";
	while(1)
	{
		char* p = abc;
		while(*p != 0)
		{
			print(p++);
			print("\n");
		}
	}
}

void calculateFibonacci()
{
	createProcess("alphabet", &printAlphabet); // we have to put it here, because otherwise it won't be executed as the irq0 switches to this task after it is added to the scheduler
	
	uint32* a = 0x1000*1024*4+2;
	
	//*a = 4567;
	print("fibonacci");
	printDec(*a);
	
	while(1)
	{
	}
	while(1)
	{
		uint32 a = 0;
		uint32 b = 1;
		uint32 c;
		int i;
		printDec(0);
		for(i = 0; i < 20; i++)
		{
			c = b;
			b = a;
			a = c + b;
			print("\n");
			printDec(a);
		}
	}
}


void kmain(multibootHeader_t *mboot_ptr)
{

	
	// descriptor tables have to be created first
	memory_init_preprotected(); // gdt
	interrupts_init(); // idt
	
	
	
	// check that our initrd was loaded by the bootloader and determine the addresses.
	ASSERT(mboot_ptr->mods_count > 0);
	uint32 initrd_location = *((uint32*)mboot_ptr->mods_addr);
	uint32 initrd_end = *(uint32*)(mboot_ptr->mods_addr+4);
	// Don't trample our module with placement accesses, please
	kmalloc_init(initrd_end);
	
	
	display_init();
	log_init();
	
	display_setColor(0x0f);
	print("\n");
	print("                                   Xelix\n");
	print("\n");
	display_setColor(0x07);
	
	
	log("Initialized preprotected memory\n"); // cheating. this already happened, but display wasn't up yet.
	log("Initialized interrupts\n");
	log("Initialized Display.\n"); 
	cpu_init();
	log("Initialized CPU\n");
	memory_init_postprotected();
	log("Initialized postprotected memory\n");
	pit_init(50); //50Hz
	log("Initialized PIT (programmable interrupt timer)\n");
	keyboard_init();
	log("Initialized keyboard\n");
	
	log("Reading Initrd...\n");
	readInitrd(initrd_location);

	print("finished listing files\n");

	display_setColor(0x0f);
	log("Xelix is up.\n");
	display_setColor(0x07);	

	print("Creating Process...\n");
	

	createProcess("fibonacci", &calculateFibonacci);
	
	while(1)
	{
		print("main kernel loop\n");
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
