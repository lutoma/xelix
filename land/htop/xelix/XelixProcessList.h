#ifndef HEADER_XelixProcessList
#define HEADER_XelixProcessList
/*
htop - XelixProcessList.h
(C) 2014 Hisham H. Muhammad
(C) 2017 Diederik de Groot
Released under the GNU GPLv2, see the COPYING file
in the source distribution for its full text.
*/

#include "XelixProcess.h"


typedef struct XelixProcessList_ {
   ProcessList super;
} XelixProcessList;

ProcessList* ProcessList_new(UsersTable* usersTable, Hashtable* pidMatchList, uid_t userId);

void ProcessList_delete(ProcessList* this);

void ProcessList_goThroughEntries(ProcessList* super, bool pauseProcessUpdate);

#endif
