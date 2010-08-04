#include <devices/display/interface.h>

uint16* const videoMemory = (uint16*) 0xB8000; // 80x25 Zeichen   // short ist zwei Bytes: 1. Byte char, 2. Byte Farben // const ist so, dass der Zeiger nicht verändert werden kann, die Daten dahinter schon

uint16* pos; //shows current cursor position

uint8 color;

void scroll(); //Scroll down one line
void moveCursor(); // Cursor richtig zum pos bewegen

/*
 *  Columns = 80
 *  Rows = 25
 */

void display_init()
{
	color = 0x0f;
	pos = videoMemory;
	display_clear();
}

void display_print(char* s)
{
	while(*s != '\0')
	{
		display_printChar(*(s++));
	}
	moveCursor();
}

void display_printChar(char c)
{
	if(c == '\n')
	{ // neue Zeile
		pos = pos - (pos-videoMemory) % 80 + 80;
	}
	else if(c== '\b') 
	{ // backspace
		pos--;
		*pos = color<<8 | ' ';
	}
	else
	{
		*(pos++) = color<<8 | c;
	}
	if(pos >= videoMemory + 80*25)
	{
		scroll();
	}
}

void display_setColor(uint8 newcolor)
{
	color = newcolor;
}

void display_printHex(uint32 num)
{
	char s[11]; // maximal 2 (0x) + 2*4 (long int sind 4 bytes) + 1 ('\0')
	s[0] = '0';
	s[1] = 'x';
	
	char tmp[9];
	int i=0;
	while(num != 0)
	{
		unsigned char c = num & 0xf;
		num = num>>4;
		if(c < 10)
			c+='0';
		else
			c= c-10 + 'A';
		tmp[i++] = c;
	}
	s[i+2] = '\0';
	int j;
	for(j=0; j < i; j++)
	{
		s[2+j] = tmp[i-1-j];
	}
	display_print(s);
}


void display_clear()
{
	int i;
	for(i=0; i<80*25; i++)
	{
		display_printChar(' ');
	}
	pos = videoMemory;
}

//Scroll the screen
void scroll()
{
	uint16* read;
	uint16* write;
	for(read = videoMemory+80, write = videoMemory; read < pos; read++, write++)
	{
		*write = *read;
	}
	
	// neue restliche Zeile leermachen sonst bleibt da die vorherige letzte Zeile
	for(; write < videoMemory+80*25; write++)
	{
		*write = color<<8 | ' ';
	}
	
	// Positionszeiger eine Zeile nach oben verschieben
	pos -= 80;
	if(pos < videoMemory)
	{
		pos = videoMemory;
	}
}

// Updates the hardware cursor.
void moveCursor()
{
   // The screen is 80 characters wide...
   uint16 cursorLocation = pos - videoMemory;
   outb(0x3D4, 14);                  // Tell the VGA board we are setting the high cursor byte.
   outb(0x3D5, cursorLocation >> 8); // Send the high cursor byte.
   outb(0x3D4, 15);                  // Tell the VGA board we are setting the low cursor byte.
   outb(0x3D5, cursorLocation);      // Send the low cursor byte.
}
