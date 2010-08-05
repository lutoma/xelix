#ifndef DRIVERS_DISPLAY_INTERFACE_H
#define DRIVERS_DISPLAY_INTERFACE_H

#include <common/generic.h>


void display_init();
void display_print(char* s);
void display_printChar(char c);
void display_printDec(uint32 num);
void display_printHex(uint32 num);
void display_setColor(uint8 newcolor);
void display_clear();
#endif
