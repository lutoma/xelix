#ifndef HEADER_XelixProcess
#define HEADER_XelixProcess
/*
htop - Xelix/XelixProcess.h
(C) 2015 Hisham H. Muhammad
(C) 2017 Diederik de Groot
Released under the GNU GPLv2, see the COPYING file
in the source distribution for its full text.
*/

typedef struct XelixProcess_ {
   Process super;
} XelixProcess;

#define Process_isKernelThread(_process) (false)

//#define Process_isUserlandThread(_process) (_process->pid != _process->tgid)
#define Process_isUserlandThread(_process) (false)

extern const ProcessClass XelixProcess_class;

extern const ProcessFieldData Process_fields[LAST_PROCESSFIELD];

Process* XelixProcess_new(const Settings* settings);

void Process_delete(Object* cast);

bool Process_isThread(const Process* this);

#endif
