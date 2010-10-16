/** @file devices/pit/generic.c
 * \brief A generic PIT driver.
 * @author Lukas Martini
 */

#include <common/log.h>
#include <devices/pit/interface.h>
#include <interrupts/interface.h>

uint32 tick = 0;

/** The timer callback. Gets called every time the PIT fires.
 * @param regs The registers supplied by the IRQ
 * @note Does nothing but increase the PIT value by one.
 */
static void timerCallback(registers_t regs)
{
	tick++;
}

/** Initialize the PIT
 * @param frequency The frequency to initialize the PIT with
 */
void pit_init(uint32 frequency)
{
	log("Initializing PIT at %d Hz.\n", frequency);
	// Firstly, register our timer callback.
	interrupt_registerHandler(IRQ0, &timerCallback);

	// The value we send to the PIT is the value to divide it's input clock
	// (1193180 Hz) by, to get our required frequency. Important to note is
	// that the divisor must be small enough to fit into 16-bits.
	uint32 divisor = 1193180 / frequency;

	// Send the command byte.
	outb(0x43, 0x36);

	// Divisor has to be sent byte-wise, so split here into upper/lower bytes.
	uint8 l = (uint8)(divisor & 0xFF);
	uint8 h = (uint8)( (divisor>>8) & 0xFF );

	// Send the frequency divisor.
	outb(0x40, l);
	outb(0x40, h);
	log("Initialized PIT (programmable interrupt timer)\n");
}

/** Get the tick num
 * @return The tick num
 */
uint32 pit_getTickNum()
{
	return tick;
}
