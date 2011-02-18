#pragma once

#include <common/generic.h>
int date(char dateStr);
char* monthToString(int month, int shortVersion);
int getWeekDay(int dayOfMonth, int month, int year);
void sleep(time_t timeout);
char* dayToString(int day, int shortVersion);
