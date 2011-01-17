// Support for internal PC speakers
#include <common/generic.h>
#ifdef WITH_SPEAKER
#include <devices/speaker/interface.h>

#include <common/log.h>
#include <common/datetime.h>

void speaker_on()
{
	outb(0x61,inb(0x61) | 3);
}

void speaker_off()
{
	outb(0x61,inb(0x61) &~3);
}

void speaker_setFrequency(uint8 frequency)
{
	uint8 divisor;
	divisor = 1193180L/frequency;
	outb(0x43,0xB6);
	outb(0x42,divisor&0xFF);
	outb(0x42,divisor >> 8);
}

void speaker_beep(uint8 frequency, time_t seconds)
{
	speaker_setFrequency(frequency);
	speaker_on();
	sleep(seconds);
	speaker_off();
}

// I still can't believe how much this function is doing
void speaker_init(){}

#endif
