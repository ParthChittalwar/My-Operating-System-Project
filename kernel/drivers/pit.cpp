#include "pit.hpp"
#include "pic.hpp"

static constexpr uint16_t PIT_CH0_DATA = 0x40;
static constexpr uint16_t PIT_CMD      = 0x43;

// Channel 0, lobyte/hibyte, mode 3 (square wave), binary
static constexpr uint8_t PIT_MODE = 0x36;

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0,%1" : : "a"(val), "Nd"(port) : "memory");
}

static volatile uint64_t s_ticks = 0;

// Called by the IRQ0 handler in idt.cpp.
extern "C" void pit_tick_handler() { s_ticks = s_ticks + 1; }

void pit_init(uint32_t hz) {
    uint32_t divisor = PIT_BASE_FREQ / hz;

    outb(PIT_CMD,      PIT_MODE);
    outb(PIT_CH0_DATA, (uint8_t)(divisor & 0xFF));
    outb(PIT_CH0_DATA, (uint8_t)(divisor >> 8));

    pic_enable_irq(0); // IRQ0 → vector 32
}

uint64_t pit_ticks() { return s_ticks; }
