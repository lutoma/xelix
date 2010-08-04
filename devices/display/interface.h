#ifndef DRIVERS_DISPLAY_INTERFACE_H
#define DRIVERS_DISPLAY_INTERFACE_H

#include <common.h>


void display_init();
void display_print(char* s);
void display_printChar(char c);
void display_printDec(int num);
void display_printHex(int num);
void display_setColor(uint8 newcolor);
void display_clear();
#endif