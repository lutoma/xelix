#ifndef DRIVERS_DISPLAY_INTERFACE_H
#define DRIVERS_DISPLAY_INTERFACE_H

#include <common/generic.h>


void display_init();
void display_printChar(char c);
void display_setColor(uint8 newcolor);

// scrolling in the buffer
void display_scrollUp();
void display_scrollDown();



#endif
