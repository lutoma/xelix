#include <devices/floppy/interface.h>
#include <memory/kmalloc.h>

floppyDrive_t drive0 = { .number = 0 }, drive1 = { .number = 1 };

static
void parseInfo(uint8 info, floppyDrive_t *dest)
{
	switch (info)
	{
		case 1:
			dest->size = 360;
			dest->inch = 525;
			break;

		case 2:
			dest->size = 1200;
			dest->inch = 525;
			break;

		case 3:
			dest->size = 720;
			dest->inch = 350;
			break;

		case 4:
			dest->size = 1440;
			dest->inch = 350;
			break;

		case 5:
			dest->size = 2880;
			dest->inch = 350;
			break;
	}
}

static void printDetails(floppyDrive_t *drive)
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
	outb(0x70, 0x10);
	uint8 c = inb(0x71);

	parseInfo(c >> 4, &drive0);
	parseInfo(c & 0xF, &drive1);

	if(drive0.size) printDetails(&drive0);
	if(drive1.size) printDetails(&drive1);
}
