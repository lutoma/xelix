// Logging functions

#include <common/log.h>
#include <common/string.h>
#include <memory/kmalloc.h>

char* kernellog;
int logsEnabled;
uint32 maxLogSize;

// Logs something. Also prints it out.
// FIXME: doesn't parse the saved stuff, needs improvement. Dirty hacks ftw.
void log(const char *fmt, ...)
{
	if(strlen(kernellog) + strlen(fmt) < maxLogSize) // prevent an overflow that is likely to happen if the log gets long enough
		kernellog = strcat(kernellog, fmt); // concatenate to kernellog
	if(logsEnabled)
		vprintf(fmt, (void **)(&fmt) + 1);
}

// Initialize log
void log_init()
{
	maxLogSize = 10000;
	kernellog = (char*)kmalloc(maxLogSize);
	kernellog[0] = '\0'; // set kernel log to empty string
	setLogLevel(1); //Enable logs
}

// Set log level
// Currently only off (0) and on (1).
void setLogLevel(int level)
{
	if(level) log("Enabled printing of log messages.\n");
	else log("Warning: disabled printing of log messages.\n");
	logsEnabled = level;
}
