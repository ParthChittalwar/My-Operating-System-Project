#include "gdt.hpp"
#include <stddef.h>

// ============================================================
// GDT Implementation — NovaOS Milestone 2
// ============================================================

// Access byte bit fields (for standard code/data descriptors):
//   Bit 7   : Present (must be 1)
//   Bits 6-5: DPL (descriptor privilege level, 0=kernel, 3=user)
//   Bit 4   : Descriptor type (1 = code/data segment)
//   Bit 3   : Executable (1 = code, 0 = data)
//   Bit 2   : Direction/Conforming
//   Bit 1   : Read/Write
//   Bit 0   : Accessed (CPU sets this; start at 0)

// Granularity byte bit fields:
//   Bit 7   : Granularity (1 = 4KB pages, 0 = bytes)
//   Bit 6   : D/B (0 for 64-bit code)
//   Bit 5   : L (Long mode — set to 1 for 64-bit code)
//   Bit 4   : Available for system use
//   Bits 3-0: Limit bits 19:16

// Access byte values:
static constexpr uint8_t ACCESS_PRESENT   = 0x80; // bit 7
static constexpr uint8_t ACCESS_RING0     = 0x00; // bits 6:5 = 00
static constexpr uint8_t ACCESS_RING3     = 0x60; // bits 6:5 = 11
static constexpr uint8_t ACCESS_SEGMENT   = 0x10; // bit 4 = code/data descriptor type
static constexpr uint8_t ACCESS_EXEC      = 0x08; // bit 3 = executable (code)
static constexpr uint8_t ACCESS_RW        = 0x02; // bit 1 = readable (code) / writable (data)
static constexpr uint8_t ACCESS_TSS_TYPE  = 0x09; // 64-bit TSS, available

// Granularity byte values:
static constexpr uint8_t GRAN_LONG_MODE  = 0x20; // bit 5 = L (64-bit code segment)
static constexpr uint8_t GRAN_PAGE       = 0x80; // bit 7 = 4KB granularity
static constexpr uint8_t GRAN_DB         = 0x40; // bit 6 = D/B (for 32-bit protected mode)

// The GDT itself — 5 standard entries + 1 system (TSS) entry
// (which occupies 2 slots due to its 16-byte size).
// Alignment to 8 bytes improves performance.
static GDTEntry     gdt[5] __attribute__((aligned(8)));
static GDTSystemEntry tss_descriptor __attribute__((aligned(8)));

// The TSS structure itself (globally allocated here).
static TSS kernel_tss __attribute__((aligned(16)));

// Reserved stack space for ring-0 (used by TSS.RSP0 in M8).
// 8KB kernel stack. For now we just point RSP0 at the end
// of a static buffer; in M3 we'll use the heap allocator.
static uint8_t kernel_stack[8192] __attribute__((aligned(16)));

// Helper: fill a standard 8-byte GDT entry.
static void set_gdt_entry(GDTEntry& entry, uint32_t base, uint32_t limit,
                          uint8_t access, uint8_t granularity) {
    entry.limit_low   = (uint16_t)(limit & 0xFFFF);
    entry.base_low    = (uint16_t)(base & 0xFFFF);
    entry.base_mid    = (uint8_t)((base >> 16) & 0xFF);
    entry.access      = access;
    entry.granularity = (granularity & 0xF0) | ((limit >> 16) & 0x0F);
    entry.base_high   = (uint8_t)((base >> 24) & 0xFF);
}

// Helper: fill the 16-byte TSS system descriptor.
static void set_tss_descriptor(GDTSystemEntry& entry, uint64_t base, uint32_t limit) {
    entry.limit_low  = (uint16_t)(limit & 0xFFFF);
    entry.base_low   = (uint16_t)(base & 0xFFFF);
    entry.base_mid   = (uint8_t)((base >> 16) & 0xFF);
    entry.access     = ACCESS_PRESENT | ACCESS_TSS_TYPE; // 0x89 = present, ring0, 64-bit TSS
    entry.granularity = ((limit >> 16) & 0x0F);          // Granularity + upper limit nibble
    entry.base_high  = (uint8_t)((base >> 24) & 0xFF);
    entry.base_upper = (uint32_t)(base >> 32);
    entry.reserved   = 0;
}

void gdt_init() {
    // --- Descriptor 0: Null (required by CPU architecture) ---
    set_gdt_entry(gdt[0], 0, 0, 0, 0);

    // --- Descriptor 1: Kernel Code (64-bit) — selector 0x08 ---
    // Access: present | ring0 | segment | executable | readable
    // Gran  : long-mode bit set, 4KB pages
    set_gdt_entry(gdt[1], 0, 0xFFFFF,
                  ACCESS_PRESENT | ACCESS_RING0 | ACCESS_SEGMENT | ACCESS_EXEC | ACCESS_RW,
                  GRAN_LONG_MODE | GRAN_PAGE);

    // --- Descriptor 2: Kernel Data — selector 0x10 ---
    // Access: present | ring0 | segment | writable (no exec bit)
    set_gdt_entry(gdt[2], 0, 0xFFFFF,
                  ACCESS_PRESENT | ACCESS_RING0 | ACCESS_SEGMENT | ACCESS_RW,
                  GRAN_PAGE | GRAN_DB);

    // --- Descriptor 3: User Code (64-bit) — selector 0x18 ---
    // Ring 3 equivalent of kernel code. Reserved for M8.
    set_gdt_entry(gdt[3], 0, 0xFFFFF,
                  ACCESS_PRESENT | ACCESS_RING3 | ACCESS_SEGMENT | ACCESS_EXEC | ACCESS_RW,
                  GRAN_LONG_MODE | GRAN_PAGE);

    // --- Descriptor 4: User Data — selector 0x20 ---
    set_gdt_entry(gdt[4], 0, 0xFFFFF,
                  ACCESS_PRESENT | ACCESS_RING3 | ACCESS_SEGMENT | ACCESS_RW,
                  GRAN_PAGE | GRAN_DB);

    // --- TSS Initialization ---
    // Zero out the TSS structure first.
    uint8_t* tss_bytes = (uint8_t*)&kernel_tss;
    for (size_t i = 0; i < sizeof(TSS); ++i) tss_bytes[i] = 0;

    // Point RSP0 to the top of our kernel stack (stack grows downward).
    // RSP0 is loaded by the CPU on ring-3 → ring-0 transitions (syscalls/interrupts from user mode).
    kernel_tss.rsp[0] = (uint64_t)(kernel_stack + sizeof(kernel_stack));

    // Set IOMAP base beyond TSS size → no I/O permission bitmap (all I/O blocked from ring 3).
    kernel_tss.iomap_base = sizeof(TSS);

    // Install TSS descriptor into our pseudo-GDT area.
    // We treat the TSS descriptor as a separate struct right after gdt[4].
    set_tss_descriptor(tss_descriptor, (uint64_t)&kernel_tss, sizeof(TSS) - 1);

    // Build an in-memory GDT layout:
    // [null | kcode | kdata | ucode | udata | tss_low | tss_high]
    // We need them contiguous. Use a packed struct on the stack for lgdt.
    // Layout: gdt[0..4] (5 × 8 bytes = 40 bytes) + tss_descriptor (16 bytes) = 56 bytes.
    struct __attribute__((packed)) FullGDT {
        GDTEntry      entries[5];
        GDTSystemEntry tss;
    };

    static FullGDT full_gdt __attribute__((aligned(8)));
    for (int i = 0; i < 5; ++i) full_gdt.entries[i] = gdt[i];
    full_gdt.tss = tss_descriptor;

    // Build GDTR and load it.
    GDTR gdtr;
    gdtr.limit = sizeof(FullGDT) - 1;
    gdtr.base  = (uint64_t)&full_gdt;

    // lgdt loads the GDTR register.
    // After lgdt, we must reload all segment registers.
    //   - DS, ES, FS, GS, SS → kernel data selector (0x10)
    //   - CS cannot be reloaded with mov; we use a far return:
    //       push  cs_selector  ; push new CS
    //       push  rip_label    ; push return address
    //       lretq               ; far return loads both CS and RIP
    asm volatile(
        "lgdt %0\n\t"
        // Reload data segments to kernel data selector (0x10)
        "mov $0x10, %%ax\n\t"
        "mov %%ax, %%ds\n\t"
        "mov %%ax, %%es\n\t"
        "mov %%ax, %%fs\n\t"
        "mov %%ax, %%gs\n\t"
        "mov %%ax, %%ss\n\t"
        // Far return to reload CS with kernel code selector (0x08).
        // We push the selector and then the return label address,
        // then lretq pops both and reloads CS:RIP atomically.
        "push $0x08\n\t"          // Push new CS (kernel code)
        "lea 1f(%%rip), %%rax\n\t" // Load address of label 1:
        "push %%rax\n\t"           // Push return RIP
        "lretq\n\t"                // Far return: pop RIP, then CS
        "1:\n\t"                   // Landing pad — we arrive here with new CS
        :
        : "m"(gdtr)
        : "rax", "memory"
    );

    // Load the TSS selector into the TR register.
    // ltr loads the task register with our TSS selector (0x28).
    asm volatile("ltr %0" : : "r"((uint16_t)GDT_TSS_SEL));
}
