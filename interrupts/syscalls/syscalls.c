#include <interrupts/interface.h>


// handles a syscall. The parameter gives the processor status for the systemcall
uint32 syscallHandler(registers_t regs)
{
	switch(regs.eax)
	{
		case 1:
			printf((char*) regs.ebx);
			return 0;
		case 2:
			printf("%d", regs.ebx);
			return 0;
		case 3:
			printf("%x", regs.ebx);
			return 0;
		default:
			printf("unknown syscall no. %d!\n", regs.eax);
			return 0;
	}
}
