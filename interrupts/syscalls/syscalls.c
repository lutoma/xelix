#include <interrupts/interface.h>


// handles a syscall. The parameter gives the processor status for the systemcall
uint32 syscallHandler(registers_t regs)
{
	switch(regs.eax)
	{
		case 1:
			print((char*) regs.ebx);
			return 0;
		case 2:
			printDec(regs.ebx);
			return 0;
		case 3:
			printHex(regs.ebx);
			return 0;
		default:
			print("unknown syscall no. ");
			printDec(regs.eax);
			print("!\n");
			return 0;
	}
}
