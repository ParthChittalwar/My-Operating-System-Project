#pragma once
#include <stdint.h>

// ============================================================
// Global Descriptor Table (GDT) — NovaOS Milestone 2
//
// The GDT defines the CPU's memory segmentation model.
// In 64-bit long mode, segmentation is largely vestigial
// (all segments have base 0 and limit 0xFFFFFFFF), but the
// GDT still matters for:
//   - Distinguishing ring 0 (kernel) from ring 3 (user)
//   - Providing a Task State Segment (TSS) for IST stacks
//   - Encoding the CS descriptor's L (long mode) bit
//
// Layout:
//   Index 0: Null descriptor     (required by CPU)
//   Index 1: Kernel Code (64-bit) — selector 0x08
//   Index 2: Kernel Data          — selector 0x10
//   Index 3: User Code  (64-bit) — selector 0x18 (placeholder for M8)
//   Index 4: User Data            — selector 0x20 (placeholder for M8)
//   Index 5-6: TSS (16-byte system descriptor)
// ============================================================

// A standard 8-byte GDT entry.
struct GDTEntry {
    uint16_t limit_low;    // Lower 16 bits of limit
    uint16_t base_low;     // Lower 16 bits of base
    uint8_t  base_mid;     // Middle 8 bits of base
    uint8_t  access;       // Access byte (type, DPL, present)
    uint8_t  granularity;  // Flags + upper 4 bits of limit
    uint8_t  base_high;    // Upper 8 bits of base
} __attribute__((packed));

// A 16-byte System Segment descriptor (used for TSS).
// The CPU treats selectors pointing to system segments
// as 16-byte entries in 64-bit mode.
struct GDTSystemEntry {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  granularity;
    uint8_t  base_high;
    uint32_t base_upper;   // Upper 32 bits of 64-bit base
    uint32_t reserved;
} __attribute__((packed));

// The GDTR register value loaded via lgdt.
struct GDTR {
    uint16_t limit;  // Size of GDT in bytes minus 1
    uint64_t base;   // Linear address of GDT
} __attribute__((packed));

// The Task State Segment (TSS) structure for x86-64.
// We use it mainly to provide RSP0 (ring-0 stack pointer
// used when entering kernel from ring 3) and IST entries
// (Interrupt Stack Table, used in M4 for safe double-fault
// handler stacks).
struct TSS {
    uint32_t reserved0;
    uint64_t rsp[3];        // RSP0, RSP1, RSP2
    uint64_t reserved1;
    uint64_t ist[7];        // IST1–IST7
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;    // I/O permission bitmap offset
} __attribute__((packed));

// Segment selector constants (byte offsets into the GDT).
// The bottom 2 bits encode RPL (Requested Privilege Level).
static constexpr uint16_t GDT_KERNEL_CODE = 0x08;
static constexpr uint16_t GDT_KERNEL_DATA = 0x10;
static constexpr uint16_t GDT_USER_CODE   = 0x18;
static constexpr uint16_t GDT_USER_DATA   = 0x20;
static constexpr uint16_t GDT_TSS_SEL     = 0x28;

// Initialize the GDT: populate descriptors, load via lgdt,
// reload segment registers, and install the TSS.
void gdt_init();
