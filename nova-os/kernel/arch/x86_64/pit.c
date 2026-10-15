/**
 * NOVA OS - Next-Generation Microkernel Operating System
 * File: kernel/arch/x86_64/pit.c
 * Description: 8254 Programmable Interval Timer (PIT) Driver.
 * Produces periodic preemptive scheduling ticks.
 * License: Apache-2.0
 */

#include "cpu.h"
#include "kprintf.h"

#define PIT_COMMAND_PORT 0x43
#define PIT_CHANNEL0_PORT 0x40
#define PIT_BASE_FREQ 1193182

static volatile uint64_t system_timer_ticks = 0;

void pit_init(uint32_t frequency_hz) {
    if (frequency_hz == 0) frequency_hz = 100;
    uint32_t divisor = PIT_BASE_FREQ / frequency_hz;

    // Command byte: Channel 0, Access mode lobyte/hibyte, Mode 3 (square wave), binary
    outb(PIT_COMMAND_PORT, 0x36);

    uint8_t low = (uint8_t)(divisor & 0xFF);
    uint8_t high = (uint8_t)((divisor >> 8) & 0xFF);

    outb(PIT_CHANNEL0_PORT, low);
    outb(PIT_CHANNEL0_PORT, high);

    kprintf("[PIT] 8254 Timer calibrated to %u Hz (divisor=%u)\n", frequency_hz, divisor);
}

void pit_tick_handler(void) {
    system_timer_ticks++;
}

uint64_t pit_get_ticks(void) {
    return system_timer_ticks;
}
