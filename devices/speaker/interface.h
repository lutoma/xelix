#pragma once

#include <common/generic.h>
#ifdef WITH_SPEAKER

void speaker_on();
void speaker_off();
void speaker_setFrequency(uint8 frequency);
void speaker_beep(uint8 frequency, time_t seconds);
void speaker_init();



#endif

