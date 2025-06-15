#include "pit.h"
#include "idt.h"

#define PIT_BASE_FREQ 1193180  // PIT base frequency in Hz
#define PIT_CHANNEL0 0x40      // Channel 0 data port
#define PIT_COMMAND 0x43       // Command port

void pit_init(uint32_t frequency) {
    // Calculate the divisor
    uint32_t divisor = PIT_BASE_FREQ / frequency;

    // Send the command byte: Channel 0, lobyte/hibyte, rate generator
    outb(PIT_COMMAND, 0x36);

    // Send the divisor (low byte, then high byte)
    outb(PIT_CHANNEL0, divisor & 0xFF);
    outb(PIT_CHANNEL0, (divisor >> 8) & 0xFF);
}