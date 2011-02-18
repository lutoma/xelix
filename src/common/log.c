// Logging functions

#include <common/log.h>
#include <common/string.h>
#include <memory/kmalloc.h>

int logsEnabled;
#ifdef LOG_SAVE
char* kernellog;
#endif

// Logs something. Also prints it out.
// FIXME: doesn't parse the saved stuff, needs improvement. Dirty hacks ftw.
void log(const char *fmt, ...)
{
	#ifdef LOG_SAVE
	if(strlen(kernellog) + strlen(fmt) < LOG_MAXSIZE) // prevent an overflow that is likely to happen if the log gets long enough
		kernellog = strcat(kernellog, fmt); // concatenate to kernellog
	#endif
	
	if(logsEnabled)
		vprintf(fmt, (void **)(&fmt) + 1);
}

// Initialize log
void log_init()
{
	#ifdef LOG_SAVE
	kernellog = (char*)kmalloc(LOG_MAXSIZE * sizeof(char));
	kernellog[0] = '\0'; // set kernel log to empty string
	#endif
	setLogLevel(1); //Enable logs
}

// Set log level
// Currently only off (0) and on (1).
void setLogLevel(int level)
{
	if(level) log("Enabled printing of log messages.\n");
	else log("log: Warning: disabled printing of log messages.\n");
	logsEnabled = level;
}
