#pragma once
#include <stdint.h>

// ============================================================
// PIT (8253/8254 Programmable Interval Timer) — M4
//
// Channel 0 drives IRQ0 at the requested frequency.
// We use this for scheduler ticks and uptime counting.
// ============================================================

// Input oscillator frequency of the 8254 (Hz).
static constexpr uint32_t PIT_BASE_FREQ = 1193180;

// Configure channel 0 to fire at `hz` interrupts per second.
// Enables IRQ0 on the PIC.
void pit_init(uint32_t hz);

// Number of timer ticks since pit_init().
uint64_t pit_ticks();
