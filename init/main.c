#include <common/generic.h>
#include <devices/display/interface.h>
#include <devices/cpu/interface.h>

void checkIntLenghts();

void checkIntLenghts()
{
	print("Checking length of uint8... ");
	if(sizeof(uint8) == 1) print("Right\n");
	else panic("Got wrong lenght for uint8");
	
	print("Checking length of uint16... ");
	if(sizeof(uint16) == 2) print("Right\n");
	else panic("Got wrong lenght for uint16");
	
	print("Checking length of uint32... ");
	if(sizeof(uint32) == 4) print("Right\n");
	else panic("Got wrong lenght for uint32");
}

void kmain()
{
	display_init();
	print("Initialized Display.\n");
	checkIntLenghts();
	cpu_init();
	print("Initialized CPU\n");


	print("Ohai! Welcome to Decore.");
	print("0test0a0aäoöuü");
}
