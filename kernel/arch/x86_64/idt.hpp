#pragma once
#include <stdint.h>

// ============================================================
// Interrupt Descriptor Table (IDT) — NovaOS Milestone 2
//
// The IDT maps interrupt/exception vectors (0–255) to handler
// functions. When the CPU receives an interrupt or exception,
// it looks up the vector in the IDT, validates the gate, and
// transfers control to the handler function.
//
// Gate types we use:
//   Interrupt Gate (type 0xE): automatically clears IF (RFLAGS.IF)
//     on entry, preventing nested hardware interrupts from
//     firing while the handler runs. Used for all ISRs and IRQs.
//
// Vectors 0–31: CPU-defined exceptions (page fault, GPF, etc.)
// Vectors 32–47: Hardware IRQs (remapped PIC or APIC — M4)
// Vectors 48+: Software-defined (syscalls — M8)
// ============================================================

// The interrupt frame pushed by the CPU (and our ISR stubs)
// before calling the C++ handler. Contains the full saved
// CPU state at time of interrupt.
struct InterruptFrame {
    // Callee-saved registers pushed by isr_stubs.asm
    uint64_t r15, r14, r13, r12, r11, r10, r9, r8;
    uint64_t rbp, rdi, rsi, rdx, rcx, rbx, rax;
    // Exception metadata (pushed by stub or CPU)
    uint64_t interrupt_number;
    uint64_t error_code;
    // CPU-pushed exception frame (hardware pushes these automatically)
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;   // Only present on ring-3 → ring-0 transition
    uint64_t ss;    // Only present on ring-3 → ring-0 transition
};

// A single 16-byte IDT gate descriptor.
struct IDTEntry {
    uint16_t offset_low;   // Lower 16 bits of handler address
    uint16_t selector;     // Code segment selector (GDT_KERNEL_CODE = 0x08)
    uint8_t  ist;          // Interrupt Stack Table index (0 = use current stack)
    uint8_t  type_attr;    // Gate type + DPL + present bit
    uint16_t offset_mid;   // Middle 16 bits of handler address
    uint32_t offset_high;  // Upper 32 bits of handler address
    uint32_t reserved;     // Must be zero
} __attribute__((packed));

// The IDTR register value loaded via lidt.
struct IDTR {
    uint16_t limit;  // Size of IDT in bytes minus 1
    uint64_t base;   // Linear address of IDT
} __attribute__((packed));

// Gate type constants for type_attr byte:
//   Bits 7  : Present
//   Bits 6-5: DPL (0 = kernel only, 3 = callable from user)
//   Bit  4  : 0 (always for gate descriptors)
//   Bits 3-0: Gate type (0xE = 64-bit interrupt gate)
static constexpr uint8_t IDT_PRESENT        = 0x80;
static constexpr uint8_t IDT_DPL_RING0      = 0x00;
static constexpr uint8_t IDT_DPL_RING3      = 0x60;
static constexpr uint8_t IDT_INTERRUPT_GATE = 0x0E;

// Human-readable names for CPU exception vectors 0–31.
// Indexed by exception vector number.
extern const char* exception_messages[32];

// Initialize the IDT: set all 32 exception gates, load via lidt.
void idt_init();

// Register a handler for a specific interrupt vector.
// handler_addr: virtual address of the ISR stub function.
// flags: type_attr byte (use IDT_PRESENT | IDT_DPL_RING0 | IDT_INTERRUPT_GATE = 0x8E).
void idt_set_gate(uint8_t vector, uint64_t handler_addr, uint8_t flags);

// The C++ exception dispatcher. Called from isr_common_stub in isr_stubs.asm.
// Receives a pointer to the InterruptFrame pushed on the stack.
extern "C" void isr_handler(InterruptFrame* frame);
