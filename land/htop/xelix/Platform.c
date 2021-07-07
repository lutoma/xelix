/*
htop - dragonflybsd/Platform.c
(C) 2014 Hisham H. Muhammad
(C) 2017 Diederik de Groot
(C) 2019-2021 Lukas Martini
Released under the GNU GPLv2, see the COPYING file
in the source distribution for its full text.
*/

#include "Platform.h"
#include "Macros.h"
#include "Meter.h"
#include "CPUMeter.h"
#include "MemoryMeter.h"
#include "SwapMeter.h"
#include "TasksMeter.h"
#include "LoadAverageMeter.h"
#include "UptimeMeter.h"
#include "ClockMeter.h"
#include "DateMeter.h"
#include "DateTimeMeter.h"
#include "HostnameMeter.h"
#include "XelixProcess.h"
#include "XelixProcessList.h"

#include <stdio.h>


const ProcessField Platform_defaultFields[] = { PID, USER, PRIORITY, NICE, M_VIRT, M_RESIDENT, STATE, PERCENT_CPU, PERCENT_MEM, TIME, COMM, 0 };

const SignalItem Platform_signals[] = {
   { .name = " 0 Cancel",    .number =  0 },
   { .name = " 1 SIGHUP",    .number =  1 },
   { .name = " 2 SIGINT",    .number =  2 },
   { .name = " 3 SIGQUIT",   .number =  3 },
   { .name = " 4 SIGILL",    .number =  4 },
   { .name = " 5 SIGTRAP",   .number =  5 },
   { .name = " 6 SIGABRT",   .number =  6 },
   { .name = " 7 SIGEMT",    .number =  7 },
   { .name = " 8 SIGFPE",    .number =  8 },
   { .name = " 9 SIGKILL",   .number =  9 },
   { .name = "10 SIGBUS",    .number = 10 },
   { .name = "11 SIGSEGV",   .number = 11 },
   { .name = "12 SIGSYS",    .number = 12 },
   { .name = "13 SIGPIPE",   .number = 13 },
   { .name = "14 SIGALRM",   .number = 14 },
   { .name = "15 SIGTERM",   .number = 15 },
   { .name = "16 SIGURG",    .number = 16 },
   { .name = "17 SIGSTOP",   .number = 17 },
   { .name = "18 SIGTSTP",   .number = 18 },
   { .name = "19 SIGCONT",   .number = 19 },
   { .name = "20 SIGCHLD",   .number = 20 },
   { .name = "21 SIGTTIN",   .number = 21 },
   { .name = "22 SIGTTOU",   .number = 22 },
   { .name = "23 SIGIO",     .number = 23 },
   { .name = "24 SIGXCPU",   .number = 24 },
   { .name = "25 SIGXFSZ",   .number = 25 },
   { .name = "26 SIGVTALRM", .number = 26 },
   { .name = "27 SIGPROF",   .number = 27 },
   { .name = "28 SIGWINCH",  .number = 28 },
   { .name = "29 SIGINFO",   .number = 29 },
   { .name = "30 SIGUSR1",   .number = 30 },
   { .name = "31 SIGUSR2",   .number = 31 },
   { .name = "32 SIGTHR",    .number = 32 },
   { .name = "33 SIGLIBRT",  .number = 33 },
};

const unsigned int Platform_numberOfSignals = ARRAYSIZE(Platform_signals);

const MeterClass* const Platform_meterTypes[] = {
   &CPUMeter_class,
   &ClockMeter_class,
   &DateMeter_class,
   &DateTimeMeter_class,
   &LoadAverageMeter_class,
   &LoadMeter_class,
   &MemoryMeter_class,
   &SwapMeter_class,
   &TasksMeter_class,
   &UptimeMeter_class,
   &BatteryMeter_class,
   &HostnameMeter_class,
   &AllCPUsMeter_class,
   &AllCPUs2Meter_class,
   &AllCPUs4Meter_class,
   &AllCPUs8Meter_class,
   &LeftCPUsMeter_class,
   &RightCPUsMeter_class,
   &LeftCPUs2Meter_class,
   &RightCPUs2Meter_class,
   &LeftCPUs4Meter_class,
   &RightCPUs4Meter_class,
   &LeftCPUs8Meter_class,
   &RightCPUs8Meter_class,
   &BlankMeter_class,
   NULL
};

void Platform_init(void) {
   /* no platform-specific setup needed */
}

void Platform_done(void) {
   /* no platform-specific cleanup needed */
}

void Platform_setBindings(Htop_Action* keys) {
   /* no platform-specific key bindings */
   (void) keys;
}

int Platform_getUptime() {
   FILE* fp = fopen("/sys/tick", "r");
   if(!fp) {
      return 0;
   }

   int uptime;
   int ticks;
   int tick_rate;

   if(fscanf(fp, "%d %d %d\n", &uptime, &ticks, &tick_rate) != 3) {
      fclose(fp);
      return 0;
   }
   return uptime;
}

void Platform_getLoadAverage(double* one, double* five, double* fifteen) {
   *one = 0;
   *five = 0;
   *fifteen = 0;
}

int Platform_getMaxPid() {
   return 65536;
}

double Platform_setCPUValues(Meter* this, int cpu) {
   (void) this;
   (void) cpu;
   return 0.0;
}

void Platform_setMemoryValues(Meter* this) {
   FILE* fp = fopen("/sys/mem_info", "r");
   if(!fp) {
      return;
   }

   unsigned int mem_total = 0, mem_used = 0;

   while(!feof(fp)) {
      char name[100];
      unsigned int value;

      if(fscanf(fp, "%100[^:]: %u\n", name, &value) != 2) {
         continue;
      }

      if(!strcmp(name, "mem_total")) {
         mem_total = value;
      } else if(!strcmp(name, "mem_used")) {
         mem_used = value;
      }
   }

   this->total = mem_total / 1024;
   this->values[0] = mem_used / 1024;
   this->values[1] = 0;
   this->values[2] = 0;
   fclose(fp);
}

void Platform_setSwapValues(Meter* this) {
   (void) this;
}

char* Platform_getProcessEnv(pid_t pid) {
   (void)pid;
   return NULL;
}

char* Platform_getInodeFilename(pid_t pid, ino_t inode) {
    (void)pid;
    (void)inode;
    return NULL;
}

FileLocks_ProcessData* Platform_getProcessLocks(pid_t pid) {
    (void)pid;
    return NULL;
}

bool Platform_getDiskIO(DiskIOData* data) {
   (void)data;
   return false;
}

bool Platform_getNetworkIO(unsigned long int* bytesReceived,
                           unsigned long int* packetsReceived,
                           unsigned long int* bytesTransmitted,
                           unsigned long int* packetsTransmitted) {
   *bytesReceived = 0;
   *packetsReceived = 0;
   *bytesTransmitted = 0;
   *packetsTransmitted = 0;
   return false;
}

void Platform_getBattery(double* percent, ACPresence* isOnAC) {
   *percent = 0;
   *isOnAC = AC_PRESENT;
}
