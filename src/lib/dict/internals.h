/* Copyright © 2011 Lukas Martini
 *
 * This file is part of Libkdict.
 *
 * Libkdict is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * Libkdict is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with Libkdict. If not, see
 * <http://www.gnu.org/licenses/>.
 */

#pragma once
 
// Compatibility for embedding in Xelix kernel
#ifndef XELIX
        #include <stdint.h>
        #include <stdio.h>
        #include <malloc.h>
        #include <string.h>

        #define _debug printf
#else /* !XELIX */
        #include <lib/generic.h>
        #include <memory/kmalloc.h>
        #include <lib/string.h>
        #include <lib/log.h>

        #define malloc kmalloc
        #define free kfree
        #define _debug log
#endif /* !XELIX */

#ifdef DEBUGMODE
        #define debug(args...) _debug("libdict: debug: " args)
#else
        #define debug(...)
#endif /* DEBUGMODE */

entry_t* _findEntry(dict_t* dict, char* name);
void _deleteEntry(entry_t* entry);
