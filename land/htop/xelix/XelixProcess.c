/*
htop - XelixProcess.c
(C) 2015 Hisham H. Muhammad
(C) 2019-2020 Lukas Martini
Released under the GNU GPL, see the COPYING file
in the source distribution for its full text.
*/

#include "Process.h"
#include "XelixProcess.h"
#include <stdlib.h>

/*{
#include "Settings.h"

#define Process_delete XelixProcess_delete

}*/

Process* XelixProcess_new(Settings* settings) {
   Process* this = xCalloc(1, sizeof(Process));
   Object_setClass(this, Class(Process));
   Process_init(this, settings);
   this->user = calloc(1, 100);
   return this;
}

void XelixProcess_delete(Object* cast) {
   Process* this = (Process*) cast;
   Object_setClass(this, Class(Process));
   Process_done((Process*)cast);

   //free(this->user);
   free(this);
}

