#include <common/generic.h>
#include <devices/display/interface.h>

void kmain()
{
	// hier ist der Kernel!
	
	// testausgabe
	short* bildschirm = (short*)0xB8000;
	*bildschirm = 0x0f << 8 | 'a';
	
	display_init();
	print(":: Display initialisiert\n");
	
	printf(":: Selbstest ob ints richtige Laengen haben.\n");
	if(sizeof(uint8) == 1)
		printf("     uint8 richtig!\n");
	if(sizeof(uint16) == 2)
		printf("     uint16 richtig!\n");
	if(sizeof(uint32) == 4)
		printf("     uint32 richtig!\n");
	
	
	uint32 v;
	asm ( "mov %%cr0, %0":"=a"(v) );
	display_print("cr0: ");
	display_printHex(v);
	if(v & 1)
		display_print(" -> protected mode already enabled");
	


}
