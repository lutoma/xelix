/** @file devices/cpu/generic.c
 * \brief Generic CPU-specific commands.
 * @author Lukas Martini
 */

#include <common/log.h>
#include <devices/cpu/interface.h>

int isProtected(); // Check if CPU is in protected mode
void setProtected(); // Switch CPU to protected mode

int protected;

/// Initialize the CPU (set it to protected mode)
void cpu_init()
{
	protected = isProtected();
	if(!protected)
	{
		setProtected();
		log("Setting CPU to protected mode.\n");
	} else
	{
		log("CPU is already in protected mode, not enabling.\n");
	}

	log("Initialized CPU\n");
}
/** Check if CPU is running in protected mode.
 * @return Bool if CPU is in protected mode.
 */
int isProtected()
{
	uint32 v;
	asm ( "mov %%cr0, %0":"=a"(v) );
	if(v & 1) return 1;
	else return 0;
}

/** Set CPU in protected mode
 * @bug Not implemented yet
 * @note Not needed so far, as grub automatically switches to protected mode.
 */
void setProtected()
{
	PANIC("Can't [yet] switch to protected mode. Use a bootloader which automatically enables protected mode such as GNU GRUB.\n");
	return;
}

/** Check if CPU is 32 bit
 * @bug Not implemented yet
 * @return Always 1 (= true)
 */
int cpu_is32Bit()
{
	return 1;
}
