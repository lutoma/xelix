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

#include "test.h"
#include <common/log.h>
#include <common/string.h>
#include <memory/kmalloc.h>

int logsEnabled;
#ifdef LOG_SAVE
char* kernelLog;
#endif

// Logs something. Also prints it out.
// FIXME: doesn't parse the saved stuff, needs improvement. Dirty hacks ftw.
void log(const char *fmt, ...)
{
	#ifdef LOG_SAVE
	if(strlen(kernelLog) + strlen(fmt) < LOG_MAXSIZE) // prevent an overflow that is likely to happen if the log gets long enough
		kernelLog = strcat(kernelLog, fmt); // concatenate to kernellog
	#endif
	
	if(logsEnabled)
		vprintf(fmt, (void **)(&fmt) + 1);
}

// Initialize log
void log_init()
{
	#ifdef LOG_SAVE
	kernelLog = (char*)kmalloc(LOG_MAXSIZE * sizeof(char));
	kernelLog[0] = '\0'; // set kernel log to empty string
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
