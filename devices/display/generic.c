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
	
	/*
	// CHANGE FONT (http://www.cs.usfca.edu/~cruse/cs686f03/newzero.cpp)
	#define GRAF_PORT 0x3CE		// for i/o to the Graphics Controller
	#define TSEQ_PORT 0x3C4		// for i/o to the VGA Timer-Sequencer

	// prologue (makes character ram accessible to the cpu)
	outw( 0x0100, TSEQ_PORT );	// enter synchronous reset
	outw( 0x0402, TSEQ_PORT );	// write only to map 2
	outw( 0x0704, TSEQ_PORT );	// use sequential addressing
	outw( 0x0300, TSEQ_PORT );	// leave synchronous reset
	outw( 0x0204, GRAF_PORT );	// select map 2 for reads
	outw( 0x0005, GRAF_PORT );	// disable odd-even addressing
	outw( 0x0006, GRAF_PORT );	// map starts at 0xA000:0000

unsigned char newglyph[ 16 ] = 	{
				0x00,	// 00000000
				0x00,	// 00000000
				0x7C,	// 01111100
				0x82,	// 10000010
				0x82,	// 10000010
				0x82,	// 10000010
				0x82,   // 10000010
				0x82,	// 10000010
				0x82,	// 10000010
				0x82,	// 10000010
				0x82,	// 10000010
				0x7C,	// 01111100
				0x00,	// 00000000
				0x00,	// 00000000
				0x00,	// 00000000
				0x00	// 00000000
				};


	// load our new image for '0' into character generator ram 
	unsigned char	*vram = (unsigned char*) 0xA0000;
	int		zlocn = '0' * 32;
	int i;
	for (i = 0; i < 16; i++) vram[ zlocn + i ] = newglyph[ i ];
	
	// epilogue (makes character ram inaccessible to the cpu)
	outw( 0x0100, TSEQ_PORT );	// enter synchronous reset
	outw( 0x0302, TSEQ_PORT );	// write to maps 0 and 1
	outw( 0x0304, TSEQ_PORT );	// use odd-even addressing
	outw( 0x0300, TSEQ_PORT );	// leave synchronous reset
	outw( 0x0004, GRAF_PORT );	// select map 0 for reads
	outw( 0x1005, GRAF_PORT );	// enable odd-even addressing
	outw( 0x0E06, GRAF_PORT );	// map starts at 0xB800:0000
	*/
	
	
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

void display_printDec(uint32 num)
{
	char s[11]; // maximal log(2^(4*8)) (long int sind 4 bytes) + 1 ('\0') = 11
	
	char tmp[9];
	int i=0;
	while(num != 0)
	{
		unsigned char c = num % 10;
		num = (num - c)/10;
		c+='0';
		tmp[i++] = c;
	}
	s[i] = '\0';
	int j;
	for(j=0; j < i; j++)
	{
		s[j] = tmp[i-1-j];
	}
	display_print(s);
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
