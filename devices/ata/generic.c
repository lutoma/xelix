/*	devices/ata/generic.c: Generic driver for ATA harddisks
 *	This file is part of Xelix. The license in COPYING applies to this file. If you did not receive such a file along with this code, you can get it from http://xelix.org.
 *	Written by:
 * 	- Lukas Martini
 * 	Todo:
 * 	- Keep track of bad sectors
 * 	Notes:
 * 	- Always read the status of a drive first before sending any data. Sending something may also modify the status, resulting in loosing the ability to check if there is any drive.
 *  - All the preprocessor variables are defined in generic.h
 */

#include <devices/ata/interface.h>
#include <devices/ata/generic.h>

int selectedDrive = -1;
int blocked = 0; // Does the device currently do anything?

void delay();
void setActiveDrive();
int getDriveStatus(int drive);
void flushCache();

// Read the Status 4 times, resulting in a 400 nanoseconds delay [one cpu io port reading takes something about 100ns]. As supposed in the ATA specifications.
void delay()
{

}

// Check if device is doing something. Returns 0/1 and sets the blocked value. 0xFF [=1] means there is no drive here. Maybe call this every x seconds in a thread?
int getDriveStatus(int drive)
{
	return 0;
}

// Select the active drive we want to use on one controller [0/1].
void setActiveDrive(int drive)
{
	int value;
	ASSERT(drive > -1 && drive < 2); // Only 0 and 1 are possible
	if(selectedDrive == drive) return;
	if(drive) value = 0xA0; // Master
	else value = 0xB0; // Slave
	outb(PRIMARY_DRIVE_SELECT, value); // Warning: untested...
	selectedDrive = drive;
}

// Flush the write cache. Normally, the drive should do this automatically, but for support of old ones, we also do it manually.
void flushCache()
{

}

// Find out if there are actually any ATA drives.
void ata_detectDrives()
{
	// Write some fancy stuff to the ports and read it back in to find out if they are right.
}

// Now init all this stuff. Called by init/main.c
void ata_init()
{

}
