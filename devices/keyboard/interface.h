#ifndef DEVICES_KEYBOARD_INTERFACE_H
#define DEVICES_KEYBOARD_INTERFACE_H

#include <common/generic.h>

void keyboard_init(); // irqs have to be set up first!
void keyboard_takeFocus(void (*func)(char));
void keyboard_leaveFocus();


#endif
