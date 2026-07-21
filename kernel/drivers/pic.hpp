#pragma once
#include <stdint.h>

// ============================================================
// 8259A Programmable Interrupt Controller (PIC) — M4
//
// The PC/AT has two cascaded 8259A PICs.
//   Master PIC: IRQ0–7   remapped to INT vectors 32–39
//   Slave  PIC: IRQ8–15  remapped to INT vectors 40–47
//
// After remapping we disable all IRQs by default and let
// individual drivers enable their specific lines.
// ============================================================

// Remap both PICs to the given vector bases, then mask all IRQs.
void pic_init(uint8_t master_offset, uint8_t slave_offset);

// Send End-Of-Interrupt signal.  Must be called at end of every IRQ handler.
// For IRQ >= 8 (slave), EOI is sent to both master and slave.
extern "C" void pic_send_eoi(uint8_t irq);

// Enable / disable a specific IRQ line (0–15).
void pic_enable_irq(uint8_t irq);
void pic_disable_irq(uint8_t irq);

// Mask all IRQ lines (useful before entering a critical section).
void pic_disable_all();
