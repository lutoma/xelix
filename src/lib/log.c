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
#include "print.h"
#include <memory/kmalloc.h>
#include <hw/pit.h>

#ifndef LOG_PRINT
	bool printlog = true;
#else
	bool printlog = LOG_PRINT ;
#endif

static char* klog;
static bool initialized = false;

// Logs something. Also prints it out.
__attribute__((optimize(0))) void log(uint32_t level, const char *fmt, ...)
{
	if(unlikely(!initialized)) return;

	if(unlikely(strlen(klog) + strlen(fmt) < LOG_MAXSIZE))
		klog = strcat(klog, fmt); // concatenate to kernellog
	
	if(printlog)
	{
		printf("[%d:%d] ", pit_getTickNum() / PIT_RATE, pit_getTickNum());
		vprintf(fmt, (void**)(&fmt) + 1);
	}
}

void log_setPrintLog(bool yesno)
{
	if(yesno) log(LOG_INFO, "Enabled printing of log messages.\n");
	else log(LOG_WARN, "log: disabled printing of log messages.\n");
	printlog = yesno;
}

// Initialize log
void log_init()
{
	klog = (char*)kmalloc(LOG_MAXSIZE * sizeof(char));
	*klog = 0;
	initialized = true;
}
