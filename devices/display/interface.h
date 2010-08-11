#ifndef DRIVERS_DISPLAY_INTERFACE_H
#define DRIVERS_DISPLAY_INTERFACE_H

#include <common/generic.h>


void display_init();
void display_print(char* s);
void display_printDec(uint32 num);
void display_printHex(uint32 num);
void display_setColor(uint8 newcolor);

// scrolling in the buffer
void display_scrollUp();
void display_scrollDown();



#endif
