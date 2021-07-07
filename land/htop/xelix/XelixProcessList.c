/*
htop - XelixProcessList.c
(C) 2014 Hisham H. Muhammad
(C) 2019-2020 Lukas Martini
Released under the GNU GPL, see the COPYING file
in the source distribution for its full text.
*/

#include "ProcessList.h"
#include "XelixProcess.h"

#include <stdlib.h>
#include <string.h>
#include <pwd.h>

ProcessList* ProcessList_new(UsersTable* usersTable, Hashtable* pidWhiteList, uid_t userId) {
   ProcessList* this = xCalloc(1, sizeof(ProcessList));
   ProcessList_init(this, Class(Process), usersTable, pidWhiteList, userId);

   return this;
}

void ProcessList_delete(ProcessList* this) {
   ProcessList_done(this);
   free(this);
}

void ProcessList_goThroughEntries(ProcessList* super, bool pauseProcessUpdate) {
   if (pauseProcessUpdate) {
      return;
   }

    FILE* fp = fopen("/sys/tasks", "r");
    if(!fp) {
        return;
    }

    super->totalTasks = 0;
    super->runningTasks = 0;

    // Drop first line
    char* data = malloc(1024);
    fgets(data, 1024, fp);
    free(data);

    while(true) {
        if(feof(fp)) {
            break;
        }

        int pid;
        int uid;
        int gid;
        int ppid;
        char cstate;
        char name[500];
        int mem;
        char _tty[30];
        char* tty = _tty;

        if(fscanf(fp, "%d %d %d %d %c \"%500[^\"]\" %d %s\n", &pid, &uid, &gid, &ppid, &cstate, name, &mem, tty) != 8) {
            continue;
        }

        bool preExisting = false;
        Process *proc = ProcessList_getProcess(super, pid, &preExisting, XelixProcess_new);

        super->totalTasks++;
        if(cstate == 'R' || cstate == 'C') {
            super->runningTasks++;
            proc->state = 'R';
        } else {
            proc->state = 'S';
        }

        proc->time = proc->time + 10;
        proc->pid  = pid;
        proc->ppid = ppid;
        proc->tgid = pid;
        proc->comm = strdup(name);
        proc->basenameOffset = 0;
        proc->updated = true;

        proc->show = true;
        proc->pgrp = super->totalTasks;
        proc->session = super->totalTasks;
        proc->tty_nr = super->totalTasks;
        proc->tpgid = 0;
        proc->st_uid = uid;
        proc->flags = 0;
        proc->processor = 0;

        proc->percent_cpu = 42.23;
        proc->percent_mem = 20.0;

        struct passwd* pwd = getpwuid(uid);
        proc->user = calloc(1, 100);
        if(pwd) {
            strncpy(proc->user, pwd->pw_name, 100);
        } else {
            itoa(uid, proc->user, 100);
        }

        proc->priority = 1;
        proc->nice = 1;
        proc->nlwp = 1;
        strncpy(proc->starttime_show, "Jun 01 ", sizeof(proc->starttime_show));
        proc->starttime_ctime = 1433116800; // Jun 01, 2015

        if(!preExisting) {
            ProcessList_add(super, proc);
        }
    }

    fclose(fp);
}
