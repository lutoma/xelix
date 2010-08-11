#include <devices/floppy/interface.h>

floppyDrive_t *drive0;
floppyDrive_t *drive1;

floppyDrive_t *detectDetails(int num);
void printDriveDetails(floppyDrive_t *drive);

floppyDrive_t *detectDetails(int num)
{
	unsigned int c;
	floppyDrive_t *thisDrive;
	thisDrive = 0;
	
	ASSERT(num < 2 && num > -1); // Currently we don't support any more than two drives
	thisDrive->number = num;
	outb(0x70, 0x10);
	c = inb(0x71);

	if(!num) c >>= 4;
	else c &= 0xF;
	switch(c)
	{
		case 0:
			return 0; // No floppy drive here...

		case 1:
			thisDrive->size = 360;
			thisDrive->inch = 525;
			break;

		case 2:
			thisDrive->size = 1200;
			thisDrive->inch = 525;
			break;

		case 3:
			thisDrive->size = 720;
			thisDrive->inch = 350;
			break;

		case 4:
			thisDrive->size = 1440;
			thisDrive->inch = 350;
			break;

		case 5:
			thisDrive->size = 2880;
			thisDrive->inch = 350;
			break;
	}
	
	return thisDrive;
}

unsigned int floppy_detect()
{
	outb(0x70, 0x10);
	return inb(0x71); // 0 = no floppy devices, everything else: there is at least one. [see detectDetails]
}

void printDriveDetails(floppyDrive_t *drive)
{
  print("Floppy drive #");
  printDec(drive->number);
  print(" has following details:\n");
  print("    Size: ");
  printDec(drive->size);
  print("\n    Inch: ");
  printDec(drive->inch);
  print("\n");
}

void floppy_init()
{
	drive0 = detectDetails(0);
	drive1 = detectDetails(1);

	printDriveDetails(drive0);
	printDriveDetails(drive1);

}
