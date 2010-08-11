#include <devices/ata/interface.h>

int selectedDrive = -1;

void setActiveDrive();

void setActiveDrive(int drive)
{
  if(selectedDrive == drive) return;
  // TODO: Send command to controller to select drive.
  // outb(foo, drive);
  selectedDrive = drive;
}

void ata_detectDrives()
{

}

void ata_init()
{

}
