// Logging functions

#include <common/log.h>
#include <common/string.h>
#include <memory/kmalloc.h>

char* kernellog;
int logsEnabled;
uint32 maxLogSize;

// Logs something. Also prints it out.
void log(char* s)
{
	if(strlen(kernellog) + strlen(s) < maxLogSize) // prevent an overflow that is likely to happen if the log gets long enough
		kernellog = strcat(kernellog, s); // concatenate to kernellog
	if(logsEnabled)
		print(s); // print it on screen
}

// Same as log, only with Integer
void logDec(uint32 num)
{
	if(num == 0)
	{
	 log("0");
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
	log(s);
}

// Same as log, only with Hex
void logHex(uint32 num)
{
	if(num == 0)
	{
		log("0x0");
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
	log(s);
}

// Initialize log
void log_init()
{
	maxLogSize = 5000;
	kernellog = (char*)kmalloc(maxLogSize);
	kernellog[0] = '\0'; // set kernel log to empty string
	setLogLevel(1); //Enable logs
}

// Set log level\n
// Currently only off (0) and on (1).
void setLogLevel(int level)
{
	if(level) log("Enabled printing of log messages.\n");
	else log("Warning: disabled printing of log messages.\n");
	logsEnabled = level;
}
