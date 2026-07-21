#include "pic.hpp"

// PIC I/O port addresses
static constexpr uint16_t PIC1_CMD  = 0x20;
static constexpr uint16_t PIC1_DATA = 0x21;
static constexpr uint16_t PIC2_CMD  = 0xA0;
static constexpr uint16_t PIC2_DATA = 0xA1;

// ICW (Initialization Command Words)
static constexpr uint8_t PIC_ICW1_INIT = 0x10; // start init sequence
static constexpr uint8_t PIC_ICW1_ICW4 = 0x01; // ICW4 needed
static constexpr uint8_t PIC_ICW4_8086 = 0x01; // 8086/88 mode

static constexpr uint8_t PIC_EOI = 0x20;

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0,%1" : : "a"(val), "Nd"(port) : "memory");
}
static inline uint8_t inb(uint16_t port) {
    uint8_t v; asm volatile("inb %1,%0" : "=a"(v) : "Nd"(port) : "memory"); return v;
}
// Small I/O delay — needed after some PIC operations.
static inline void io_wait() { outb(0x80, 0); }

void pic_init(uint8_t master_offset, uint8_t slave_offset) {
    // Save current masks.
    uint8_t m1 = inb(PIC1_DATA);
    uint8_t m2 = inb(PIC2_DATA);
    (void)m1; (void)m2;

    // Start initialisation sequence (cascade mode).
    outb(PIC1_CMD,  PIC_ICW1_INIT | PIC_ICW1_ICW4); io_wait();
    outb(PIC2_CMD,  PIC_ICW1_INIT | PIC_ICW1_ICW4); io_wait();

    // ICW2: vector offsets.
    outb(PIC1_DATA, master_offset); io_wait();
    outb(PIC2_DATA, slave_offset);  io_wait();

    // ICW3: cascading.
    outb(PIC1_DATA, 0x04); io_wait(); // master: slave on IRQ2 (bit 2)
    outb(PIC2_DATA, 0x02); io_wait(); // slave:  cascade identity = 2

    // ICW4: 8086 mode.
    outb(PIC1_DATA, PIC_ICW4_8086); io_wait();
    outb(PIC2_DATA, PIC_ICW4_8086); io_wait();

    // Mask all IRQs; drivers enable their lines explicitly.
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}

void pic_send_eoi(uint8_t irq) {
    if (irq >= 8) outb(PIC2_CMD, PIC_EOI);
    outb(PIC1_CMD, PIC_EOI);
}

void pic_enable_irq(uint8_t irq) {
    uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    uint8_t  bit  = irq % 8;
    outb(port, inb(port) & ~(1u << bit));
    // If enabling a slave IRQ, also unmask IRQ2 (cascade) on master.
    if (irq >= 8) outb(PIC1_DATA, inb(PIC1_DATA) & ~(1u << 2));
}

void pic_disable_irq(uint8_t irq) {
    uint16_t port = (irq < 8) ? PIC1_DATA : PIC2_DATA;
    uint8_t  bit  = irq % 8;
    outb(port, inb(port) | (1u << bit));
}

void pic_disable_all() {
    outb(PIC1_DATA, 0xFF);
    outb(PIC2_DATA, 0xFF);
}
