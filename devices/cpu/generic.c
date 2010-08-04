#include <devices/cpu/interface.h>

int isProtected(); // Check if CPU is in protected mode
void setProtected(); // Switch CPU to protected mode

int protected;

void cpu_init()
{
	protected = isProtected();
	if(!protected)
	{
		setProtected();
		print("Setting CPU to protected mode.\n");
	} else
	{
		print("CPU is already in protected mode, not enabling.\n");
	}
}

int isProtected()
{
	uint32 v;
	asm ( "mov %%cr0, %0":"=a"(v) );
	if(v & 1) return 1;
	else return 0;
}

// Not needed so far, as grub automatically switches to protected mode.
void setProtected()
{
	panic("Can't [yet] switch to protected mode. Use a bootloader which automatically enables protected mode such as GNU GRUB.\n");
	return;
}

int cpu_is32Bit()
{
	return 1;
}
