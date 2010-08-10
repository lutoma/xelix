#include <devices/pci/interface.h>

void getDevices();

unsigned short readConfig (unsigned short bus, unsigned short slot,
										 unsigned short func, unsigned short offset)
{
	unsigned long address;
	unsigned long lbus = (unsigned long)bus;
	unsigned long lslot = (unsigned long)slot;
	unsigned long lfunc = (unsigned long)func;
	unsigned short tmp = 0;

	/* create configuration address */
	address = (unsigned long)((lbus << 16) | (lslot << 11) |
				(lfunc << 8) | (offset & 0xfc) | ((uint32)0x80000000));

	/* write out the address */
	outb(0xCF8, address);
	/* read in the data */
	tmp = (unsigned short)((inb(0xCFC) >> ((offset & 2) * 8)) & 0xffff);
	return (tmp);
}
void pci_init()
{
  // Dummy
  return;
}
