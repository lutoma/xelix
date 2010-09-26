/** @file devices/display/generic.c
 * \brief Generic display driver
 * @author Lukas Martini
 * @author Christoph Sünderhauf
 */

#include <devices/display/interface.h>
#include <memory/kmalloc.h>

const uint32 columns = 80; // on-screen character grid
const uint32 rows = 25;

/** Pointer to video memory\n
 * 80x25 Zeichen\n
 * short ist zwei Bytes: 1. Byte char, 2. Byte Farben\n
 * const ist so, dass der Zeiger nicht verändert werden kann, die Daten dahinter schon
 */
uint16* const videoMemory = (uint16*) 0xB8000; 


/**
 * Buffer concept:\n
 * Everything is written to the buffer (which is x number of lines).\n
 * The screen displays part of the buffer.\n
 * The buffer is wrap-around, i.e. when the end is reached it just continues at the beginning. (see also wrapAroundBuffer())
 * 
 */
uint16* buffer; // start of the buffer
uint16* bufferEnd; // end of the buffer (points one beyond last character)
uint16* screenPos; // points to the first character (which is a first character in a line) that is currently displayed on the screen.
uint16* cursorPos; // points to current cursor position in buffer. This is directly after the last printed character. This is where new text is printed etc. (use wrapAroundBuffer() to ensure it does not point outside the buffer)


uint8 color; // the current color


void printChar(char c); // prints a char    should not be used directly, use display_print() instead

// wraps the given position in the buffer
uint16* wrapAroundBuffer(uint16* pos);

// copies the buffer to screen (starting at screenPos)
void copyBufferToScreen();

void updateCursorPosition();

void display_clear();

/// Clear screen
void display_clear()
{
	// clear the screen
	uint16* p = videoMemory;
	int i;
	for(i=0; i<80*25; i++)
	{
		*p = color<<8 | ' ';
		p++;
	}
	
	// clear buffer
	uint16* ptr;
	for(ptr = buffer; ptr < bufferEnd; ptr++)
	{
		*ptr = color<<8 | ' ';
	}
	
	screenPos = buffer;
	cursorPos = buffer + (80*25);
}

/// Initialize display
void display_init()
{
	color = 0x07;
	
	uint32 bufferSize = columns * rows * 5; // number of characters in buffer
	buffer = kmalloc(sizeof(uint16) * bufferSize);
	bufferEnd = buffer + bufferSize;

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
	*videoMemory = color<<8 | 'P';
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
	outw( 0x0E06, GRAF_PORT );	// map starts at 0http://www.flickr.com/photos/jungepiraten/4844152708/sizes/l/xB800:0000
	*/
	
	
}

/// The main print function which should always be used
void display_print(char* s)
{
	while(*s != '\0')
	{
		printChar(*(s++));
	}
	
	// automatically set screenPos so the user sees the new content
	screenPos = cursorPos;
	screenPos = screenPos - (screenPos-buffer) % columns;
	screenPos = screenPos - columns*(rows-1);
	screenPos = wrapAroundBuffer(screenPos);
	
	copyBufferToScreen();
	updateCursorPosition();
}

/// Print single char. Mostly used internally
void printChar(char c)
{
	if(c == '\n')
	{ // new line
		cursorPos = cursorPos - (cursorPos-buffer) % columns + columns; // advance to next line
		cursorPos = wrapAroundBuffer(cursorPos);
		// fill new line with blanks (we might be reusing part of the buffer)
		uint16* ptr;
		for(ptr = cursorPos; ptr < cursorPos + columns; ptr++)
			*ptr = color<<8 | ' ';
	}
	else if(c== '\b') 
	{ // backspace
		cursorPos--;
		cursorPos = wrapAroundBuffer(cursorPos);
		*cursorPos = color<<8 | ' ';
	}
	else if(c== '\t')
	{ // tab
		uint16* oldCursorPos = cursorPos;
		cursorPos = cursorPos - (cursorPos-buffer) % 4 + 4; // advance to next tabstop
		cursorPos = wrapAroundBuffer(cursorPos);
		// clear where the cursor advanced
		while(oldCursorPos != cursorPos)
		{
			*oldCursorPos = color<<8 | ' ';
			oldCursorPos++;
			oldCursorPos = wrapAroundBuffer(oldCursorPos);
		}
	}
	else
	{
		// normal character
		*cursorPos = color<<8 | c;
		cursorPos++;
		cursorPos = wrapAroundBuffer(cursorPos);
	}
}

/** Wrap the buffer
 * @return the position
 */
inline uint16* wrapAroundBuffer(uint16* pos)
{
	if(pos >= bufferEnd)
		return pos - ( bufferEnd - buffer );
	if(pos < buffer)
		return pos + ( bufferEnd - buffer );
	return pos;
}

/// Copy the buffer to the screen
void copyBufferToScreen()
{
	// copy correct part of buffer to screen
	uint16* from = screenPos;
	uint16* to = videoMemory;
	uint32 count = columns*rows;
	while(count > 0)
	{
		*to = *from;
		to++;
		from = wrapAroundBuffer(++from);
		count--;
	}
}

// Update the cursor position on the screen
void updateCursorPosition()
{
	// set cursor Position
	
	uint16* screenVirt = screenPos; // the screenPos mapped so that it is always before cursorPos, even if it is infront of the buffer
	if(screenVirt > cursorPos)
		screenVirt -= bufferEnd-buffer;
	
	uint16 cursorLocation = columns*rows; // relative position of cursor, initialise to outside of screen -> cursor not visible
	
	if(cursorPos-screenVirt < columns*rows)
	{ // cursor is in visible screen
		cursorLocation = cursorPos-screenVirt;
	}
	
	outb(0x3D4, 14);                  // Tell the VGA board we are setting the high cursor byte.
	outb(0x3D5, cursorLocation >> 8); // Send the high cursor byte.
	outb(0x3D4, 15);                  // Tell the VGA board we are setting the low cursor byte.
	outb(0x3D5, cursorLocation);      // Send the low cursor byte.
}

/// Scroll up the display
void display_scrollUp()
{
	screenPos -= columns;
	screenPos = wrapAroundBuffer(screenPos);
	
	copyBufferToScreen();
	updateCursorPosition();
}

/// Scroll down the display
void display_scrollDown()
{
	screenPos += columns;
	screenPos = wrapAroundBuffer(screenPos);
	
	copyBufferToScreen();
	updateCursorPosition();
}

/// Set display color
void display_setColor(uint8 newcolor)
{
	color = newcolor;
}


/// Print numbers
void display_printDec(uint32 num)
{
	if(num == 0)
	{
		display_print("0");
		return;
	}
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

/// Display number in hexadecimal form
void display_printHex(uint32 num)
{
	if(num == 0)
	{
		display_print("0x0");
		return;
	}
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
