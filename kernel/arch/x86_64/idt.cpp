#include "idt.hpp"
#include "gdt.hpp"
#include "../../lib/panic.hpp"
#include <stddef.h>

// ============================================================
// IDT Implementation — NovaOS Milestone 2
// ============================================================

// The 256-entry IDT.  We only populate the first 32 entries
// (CPU exceptions) here.  Hardware IRQs (vectors 32–47)
// are added in Milestone 4 when we configure the PIC/APIC.
static IDTEntry idt[256] __attribute__((aligned(16)));

// Exception names for diagnostic output (vectors 0–31).
const char* exception_messages[32] = {
    "#DE: Divide Error",                    // 0
    "#DB: Debug Exception",                 // 1
    "NMI Interrupt",                        // 2
    "#BP: Breakpoint",                      // 3
    "#OF: Overflow",                        // 4
    "#BR: BOUND Range Exceeded",            // 5
    "#UD: Invalid Opcode",                  // 6
    "#NM: Device Not Available",            // 7
    "#DF: Double Fault",                    // 8
    "Coprocessor Segment Overrun",          // 9
    "#TS: Invalid TSS",                     // 10
    "#NP: Segment Not Present",             // 11
    "#SS: Stack-Segment Fault",             // 12
    "#GP: General Protection Fault",        // 13
    "#PF: Page Fault",                      // 14
    "Reserved",                             // 15
    "#MF: x87 FPU Error",                   // 16
    "#AC: Alignment Check",                 // 17
    "#MC: Machine Check",                   // 18
    "#XM: SIMD Floating-Point Exception",   // 19
    "#VE: Virtualization Exception",        // 20
    "#CP: Control Protection Exception",    // 21
    "Reserved",                             // 22
    "Reserved",                             // 23
    "Reserved",                             // 24
    "Reserved",                             // 25
    "Reserved",                             // 26
    "Reserved",                             // 27
    "#HV: Hypervisor Injection",            // 28
    "#VC: VMM Communication",               // 29
    "#SX: Security Exception",              // 30
    "Reserved"                              // 31
};

// External ISR stub symbols declared in isr_stubs.asm.
// Each isr_stub_N is a NASM procedure that pushes the
// exception number and a dummy error code (if needed),
// then jumps to isr_common_stub.
extern "C" {
    void isr_stub_0();  void isr_stub_1();  void isr_stub_2();  void isr_stub_3();
    void isr_stub_4();  void isr_stub_5();  void isr_stub_6();  void isr_stub_7();
    void isr_stub_8();  void isr_stub_9();  void isr_stub_10(); void isr_stub_11();
    void isr_stub_12(); void isr_stub_13(); void isr_stub_14(); void isr_stub_15();
    void isr_stub_16(); void isr_stub_17(); void isr_stub_18(); void isr_stub_19();
    void isr_stub_20(); void isr_stub_21(); void isr_stub_22(); void isr_stub_23();
    void isr_stub_24(); void isr_stub_25(); void isr_stub_26(); void isr_stub_27();
    void isr_stub_28(); void isr_stub_29(); void isr_stub_30(); void isr_stub_31();
    void irq_stub_0();  void irq_stub_1();
}

// Lookup table: ISR stub addresses (avoids a giant switch).
static void* isr_stub_table[32] = {
    (void*)isr_stub_0,  (void*)isr_stub_1,  (void*)isr_stub_2,  (void*)isr_stub_3,
    (void*)isr_stub_4,  (void*)isr_stub_5,  (void*)isr_stub_6,  (void*)isr_stub_7,
    (void*)isr_stub_8,  (void*)isr_stub_9,  (void*)isr_stub_10, (void*)isr_stub_11,
    (void*)isr_stub_12, (void*)isr_stub_13, (void*)isr_stub_14, (void*)isr_stub_15,
    (void*)isr_stub_16, (void*)isr_stub_17, (void*)isr_stub_18, (void*)isr_stub_19,
    (void*)isr_stub_20, (void*)isr_stub_21, (void*)isr_stub_22, (void*)isr_stub_23,
    (void*)isr_stub_24, (void*)isr_stub_25, (void*)isr_stub_26, (void*)isr_stub_27,
    (void*)isr_stub_28, (void*)isr_stub_29, (void*)isr_stub_30, (void*)isr_stub_31,
};

void idt_set_gate(uint8_t vector, uint64_t handler_addr, uint8_t flags) {
    IDTEntry& entry = idt[vector];
    entry.offset_low  = (uint16_t)(handler_addr & 0xFFFF);
    entry.selector    = GDT_KERNEL_CODE;         // Always kernel code segment
    entry.ist         = 0;                        // Use current RSP (IST in M4)
    entry.type_attr   = flags;
    entry.offset_mid  = (uint16_t)((handler_addr >> 16) & 0xFFFF);
    entry.offset_high = (uint32_t)(handler_addr >> 32);
    entry.reserved    = 0;
}

void idt_init() {
    // Zero out all 256 entries first (ensures unused vectors are
    // marked not-present and won't cause spurious faults).
    uint8_t* idt_bytes = (uint8_t*)idt;
    for (size_t i = 0; i < sizeof(idt); ++i) idt_bytes[i] = 0;

    // Register all 32 CPU exception handlers.
    // Flags: 0x8E = present | ring0 | interrupt gate (clears IF on entry).
    constexpr uint8_t EXCEPTION_FLAGS = IDT_PRESENT | IDT_DPL_RING0 | IDT_INTERRUPT_GATE;
    for (int i = 0; i < 32; ++i) {
        idt_set_gate((uint8_t)i, (uint64_t)isr_stub_table[i], EXCEPTION_FLAGS);
    }

    // Register IRQ0 (vector 32: PIT) and IRQ1 (vector 33: Keyboard)
    idt_set_gate(32, (uint64_t)irq_stub_0, EXCEPTION_FLAGS);
    idt_set_gate(33, (uint64_t)irq_stub_1, EXCEPTION_FLAGS);

    // Build the IDTR and load it via lidt.
    IDTR idtr;
    idtr.limit = sizeof(idt) - 1;
    idtr.base  = (uint64_t)idt;
    asm volatile("lidt %0" : : "m"(idtr));
}

// ============================================================
// C++ Exception Dispatcher
// Called from isr_common_stub in isr_stubs.asm with a pointer
// to the InterruptFrame saved on the stack.
// ============================================================

// Simple hex formatter for register dump.
// Writes a 64-bit value as "0x0000000000000000\0" into buf (19 bytes).
static void u64_to_hex(uint64_t val, char* buf) {
    buf[0] = '0'; buf[1] = 'x';
    static const char hex[] = "0123456789ABCDEF";
    for (int i = 15; i >= 2; --i) {
        buf[i] = hex[val & 0xF];
        val >>= 4;
    }
    buf[18] = '\0';
}

// Forward declaration of serial print (defined in kernel_main.cpp).
// In M3 we'll move this into a dedicated serial driver.
extern "C" void print_serial(const char* str);

extern "C" void isr_handler(InterruptFrame* frame) {
    // Print the panic header.
    print_serial("\n\n--- KERNEL PANIC ---\n");

    // Print exception name.
    uint64_t vec = frame->interrupt_number;
    if (vec < 32) {
        print_serial("Exception: ");
        // Print vector number as hex (e.g. "0x0E")
        static const char hex[] = "0123456789ABCDEF";
        char vec_str[5];
        vec_str[0] = '0'; vec_str[1] = 'x';
        vec_str[2] = hex[(vec >> 4) & 0xF];
        vec_str[3] = hex[vec & 0xF];
        vec_str[4] = '\0';
        print_serial(vec_str);
        print_serial(" ");
        print_serial(exception_messages[vec]);
        print_serial("\n");
    } else {
        print_serial("Unknown interrupt vector\n");
    }

    // Register dump.
    char hex_buf[20];

    print_serial("Error Code: ");
    u64_to_hex(frame->error_code, hex_buf);
    print_serial(hex_buf); print_serial("\n");

    print_serial("RIP: ");
    u64_to_hex(frame->rip, hex_buf);
    print_serial(hex_buf);
    print_serial("  CS: ");
    u64_to_hex(frame->cs, hex_buf);
    print_serial(hex_buf); print_serial("\n");

    print_serial("RFLAGS: ");
    u64_to_hex(frame->rflags, hex_buf);
    print_serial(hex_buf); print_serial("\n");

    print_serial("RAX: "); u64_to_hex(frame->rax, hex_buf); print_serial(hex_buf); print_serial("  ");
    print_serial("RBX: "); u64_to_hex(frame->rbx, hex_buf); print_serial(hex_buf); print_serial("\n");
    print_serial("RCX: "); u64_to_hex(frame->rcx, hex_buf); print_serial(hex_buf); print_serial("  ");
    print_serial("RDX: "); u64_to_hex(frame->rdx, hex_buf); print_serial(hex_buf); print_serial("\n");
    print_serial("RSI: "); u64_to_hex(frame->rsi, hex_buf); print_serial(hex_buf); print_serial("  ");
    print_serial("RDI: "); u64_to_hex(frame->rdi, hex_buf); print_serial(hex_buf); print_serial("\n");
    print_serial("RBP: "); u64_to_hex(frame->rbp, hex_buf); print_serial(hex_buf); print_serial("  ");
    print_serial("RSP: "); u64_to_hex(frame->rsp,  hex_buf); print_serial(hex_buf); print_serial("\n");
    print_serial("R8 : "); u64_to_hex(frame->r8,  hex_buf); print_serial(hex_buf); print_serial("  ");
    print_serial("R9 : "); u64_to_hex(frame->r9,  hex_buf); print_serial(hex_buf); print_serial("\n");
    print_serial("R10: "); u64_to_hex(frame->r10, hex_buf); print_serial(hex_buf); print_serial("  ");
    print_serial("R11: "); u64_to_hex(frame->r11, hex_buf); print_serial(hex_buf); print_serial("\n");
    print_serial("R12: "); u64_to_hex(frame->r12, hex_buf); print_serial(hex_buf); print_serial("  ");
    print_serial("R13: "); u64_to_hex(frame->r13, hex_buf); print_serial(hex_buf); print_serial("\n");
    print_serial("R14: "); u64_to_hex(frame->r14, hex_buf); print_serial(hex_buf); print_serial("  ");
    print_serial("R15: "); u64_to_hex(frame->r15, hex_buf); print_serial(hex_buf); print_serial("\n");

    // Call kernel_panic to draw red screen and halt.
    kernel_panic("Unhandled CPU exception. System halted.");
}
