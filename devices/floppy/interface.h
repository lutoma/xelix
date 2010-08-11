#ifndef DEVICES_FLOPPY_INTERFACE_H
#define EVICES_FLOPPY_INTERFACE_H

#include <common/generic.h>
struct floppyDrive;

typedef struct floppyDrive
{
  int number;
  size_t size;
  size_t inch;
} floppyDrive_t;


unsigned int floppy_detect(); // Detect if there are any floppy drives
void floppy_init(); // If yes, init them

#endif
