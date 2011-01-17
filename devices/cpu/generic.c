//Generic CPU-specific commands.
#include <devices/cpu/interface.h>

#include <common/log.h>

int isProtected(); // Check if CPU is in protected mode
void setProtected(); // Switch CPU to protected mode

int protected;

// Check if CPU is running in protected mode.
int isProtected()
{
	uint32 v;
	asm ( "mov %%cr0, %0":"=a"(v) );
	if(v & 1) return 1;
	else return 0;
}

// Set CPU in protected mode
// Todo: Implement it
void setProtected()
{
	PANIC("Can't [yet] switch to protected mode. Use a bootloader which automatically enables protected mode such as GNU GRUB.\n");
	return;
}

// Check if CPU is 32 bit
// Todo: Implement it
int cpu_is32Bit()
{
	return 1;
}

// Initialize the CPU (set it to protected mode)
void cpu_init()
{
	protected = isProtected();
	if(!protected)
	{
		setProtected();
		log("cpu: Setting CPU to protected mode.\n");
	} else
	{
		log("cpu: Already in protected mode, not enabling.\n");
	}
}
