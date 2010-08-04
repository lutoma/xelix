#include <common/generic.h>
#include <devices/display/interface.h>

void kmain()
{
	// hier ist der Kernel!
	
	// testausgabe
	short* bildschirm = (short*)0xB8000;
	*bildschirm = 0x0f << 8 | 'a';
	
	display_init();
	display_print("Display initialisiert.\n");
	
	display_print("Selbstest ob ints richtige Laengen haben.\n");
	if(sizeof(uint8) == 1)
		display_print("     uint8 richtig!\n");
	if(sizeof(uint16) == 2)
		display_print("     uint16 richtig!\n");
	if(sizeof(uint32) == 4)
		display_print("     uint32 richtig!\n");
	


}
