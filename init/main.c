#include <common/generic.h>
#include <devices/display/interface.h>
#include <devices/cpu/interface.h>

void kmain()
{
	display_init();
	print("Initialized Display.\n");

	print("Checking length of uint8... ");
	if(sizeof(uint8) == 1) print("Right\n");
	else print("WRONG!\n");
	print("Checking length of uint16... ");
	if(sizeof(uint16) == 2) print("Right\n");
	else print("WRONG!\n");
	print("Checking length of uint32... ");
	if(sizeof(uint32) == 4) print("Right\n");
	else print("WRONG!\n");

	cpu_init();
	print("Initialized CPU\n");
}
