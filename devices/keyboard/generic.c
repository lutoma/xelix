#include <devices/keyboard/interface.h>
#include <devices/display/interface.h>


// modifier keys
struct {
	int shift:1;
	int control:1;
	int alt:1;
	int super:1;
} modifiers;

void handleScancode(uint8 code);
void printModifiers();

// init after interrupts have been initialised
void keyboard_init()
{
	modifiers.shift = 0;
	modifiers.control = 0;
	modifiers.alt = 0;
	modifiers.super = 0;
	
	// TODO: replace by irq handler
	uint8 oldin = 0;
	while(1)
	{
		uint8 in = inb(0x60);
		if(in != oldin)
		{
			oldin = in;
			handleScancode(in);
		}
	}
		
}

void handleScancode(uint8 code)
{
	uint8 code2 = 0;
	if (code == 0xe0) // escape sequence
		code2 = inb(0x60);
	
	print(" Keycode ");
	display_printHex(code);
	
	if( code==0x2a) // shift press
		modifiers.shift=1;
	if( code==0xaa) // shift release
		modifiers.shift=0;
	if( code==0x1d) // ctrl press
		modifiers.control=1;
	if( code==0x9d) // control release
		modifiers.control=0;
	if( code==0x38) // alt press
		modifiers.alt = 1;
	if( code==0xb8) // alt release
		modifiers.alt=0;
	
	printModifiers();

}

void printModifiers()
{
	print(" Modifiers: ");
	if(modifiers.shift)
		print("shift ");
	if(modifiers.control)
		print("control ");
	if(modifiers.alt)
		print("alt ");
	if(modifiers.super)
		print("super ");
}


