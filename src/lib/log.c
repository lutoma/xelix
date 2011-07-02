/* log.c: Kernel logger
 * Copyright © 2010, 2011 Lukas Martini
 *
 * This file is part of Xelix.
 *
 * Xelix is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Xelix is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Xelix.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "log.h"
#include "string.h"
#include <memory/kmalloc.h>

#define DEFAULT_LEVEL LOG_INFO
#define DEFAULT_PRINTLOG true

char* kernelLog;

bool printLog;
uint32_t logLevel;

// Logs something. Also prints it out.
// FIXME: doesn't parse the saved stuff, needs improvement. Dirty hacks ftw.
void log(uint32_t level, const char *fmt, ...)
{
	if(level > logLevel)
		return;
	
	if(strlen(kernelLog) + strlen(fmt) < LOG_MAXSIZE) // prevent an overflow that is likely to happen if the log gets long enough
		kernelLog = strcat(kernelLog, fmt); // concatenate to kernellog
	
	if(printLog)
		vprintf(fmt, (void **)(&fmt) + 1);
}

void log_setLogLevel(uint32_t level)
{
	logLevel = level;
	log(LOG_INFO, "log: Set log level to %d.\n", level);
}

void log_setPrintLog(bool yesno)
{
	if(yesno) log(LOG_INFO, "Enabled printing of log messages.\n");
	else log(LOG_WARN, "log: disabled printing of log messages.\n");
	printLog = yesno;
}

// Initialize log
void log_init()
{
	kernelLog = (char*)kmalloc(LOG_MAXSIZE * sizeof(char));
	kernelLog[0] = '\0'; // set kernel log to empty string
	log_setLogLevel(DEFAULT_LEVEL);
	log_setPrintLog(DEFAULT_PRINTLOG);
}
