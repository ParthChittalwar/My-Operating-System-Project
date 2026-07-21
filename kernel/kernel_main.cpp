#include <stdint.h>
#include <stddef.h>
#include "limine.h"
#include "arch/x86_64/gdt.hpp"
#include "arch/x86_64/idt.hpp"
#include "lib/panic.hpp"
#include "mm/pmm.hpp"
#include "mm/heap.hpp"
#include "drivers/pic.hpp"
#include "drivers/pit.hpp"
#include "drivers/keyboard.hpp"
#include "drivers/framebuffer.hpp"
#include "sched/scheduler.hpp"
#include "fs/vfs.hpp"
#include "shell/shell.hpp"

// ============================================================
// Limine Protocol Requests
// ============================================================

static volatile LIMINE_BASE_REVISION(2);

static volatile struct limine_framebuffer_request framebuffer_request = {
    .id = LIMINE_FRAMEBUFFER_REQUEST,
    .revision = 0,
    .response = NULL
};

static volatile struct limine_memmap_request memmap_request = {
    .id = LIMINE_MEMMAP_REQUEST,
    .revision = 0,
    .response = NULL
};

static volatile struct limine_hhdm_request hhdm_request = {
    .id = LIMINE_HHDM_REQUEST,
    .revision = 0,
    .response = NULL
};

// ============================================================
// Low-Level Port I/O (COM1 Serial Output)
// ============================================================

static inline void outb(uint16_t port, uint8_t val) {
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port) : "memory");
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port) : "memory");
    return ret;
}

static void init_serial() {
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x80);
    outb(0x3F8 + 0, 0x03); // 38400 baud
    outb(0x3F8 + 1, 0x00);
    outb(0x3F8 + 3, 0x03);
    outb(0x3F8 + 2, 0xC7);
    outb(0x3F8 + 4, 0x0B);
}

static int is_transmit_empty() { return inb(0x3F8 + 5) & 0x20; }

static void write_serial_char(char c) {
    while (is_transmit_empty() == 0);
    outb(0x3F8, c);
}

extern "C" void print_serial(const char* str) {
    for (size_t i = 0; str[i] != '\0'; ++i) {
        if (str[i] == '\n') write_serial_char('\r');
        write_serial_char(str[i]);
    }
}

// Background task to demonstrate scheduler round-robin execution
static void background_ticker_task() {
    for (;;) {
        for (int i = 0; i < 50000000; ++i) {
            asm volatile("nop");
        }
    }
}

// ============================================================
// Kernel Entry Point
// ============================================================

extern "C" void _start(void) {
    // 1. Serial logging
    init_serial();
    print_serial("\n=== NovaOS Kernel Booting ===\n");

    // 2. GDT Setup
    gdt_init();
    print_serial("[M2] GDT loaded (Null, KCode, KData, UCode, UData, TSS).\n");

    // 3. Limine checks
    if (LIMINE_BASE_REVISION_SUPPORTED == false) {
        kernel_panic("Limine base revision unsupported!");
    }

    // 4. Framebuffer acquisition
    if (framebuffer_request.response == nullptr || framebuffer_request.response->framebuffer_count < 1) {
        kernel_panic("No framebuffer provided by bootloader!");
    }
    struct limine_framebuffer* fb = framebuffer_request.response->framebuffers[0];
    set_panic_framebuffer((void*)fb->address, (uint32_t)fb->width, (uint32_t)fb->height, (uint32_t)fb->pitch);
    fb_init((void*)fb->address, (uint32_t)fb->width, (uint32_t)fb->height, (uint32_t)fb->pitch);
    print_serial("[M1] GOP Framebuffer initialized.\n");

    // 5. Memory Management (PMM + Heap)
    if (memmap_request.response != nullptr && hhdm_request.response != nullptr) {
        pmm_init(
            hhdm_request.response->offset,
            memmap_request.response->entries,
            memmap_request.response->entry_count
        );
        print_serial("[M3] PMM initialized (Bitmap physical allocator).\n");
        heap_init();
        print_serial("[M3] Kernel Heap initialized (kmalloc/kfree ready).\n");
    } else {
        print_serial("[M3] Warning: Memory map or HHDM unavailable.\n");
    }

    // 6. PIC Remap & Timer/Keyboard Drivers
    pic_init(32, 40);
    print_serial("[M4] PIC remapped (Master: 32, Slave: 40).\n");
    pit_init(100);
    print_serial("[M4] PIT timer configured at 100 Hz.\n");
    keyboard_init();
    print_serial("[M4] PS/2 Keyboard driver active.\n");

    // 7. Interrupt Descriptor Table (IDT)
    idt_init();
    print_serial("[M2/M4] IDT installed (32 Exceptions + IRQ0 + IRQ1).\n");

    // 8. Virtual Filesystem
    vfs_init();
    print_serial("[M6] Virtual File System mounted (RAM Disk).\n");

    // 9. Multitasking Scheduler
    sched_init();
    sched_create_task(background_ticker_task, "ticker");
    print_serial("[M5] Round-robin scheduler active.\n");

    // 10. Shell Subsystem
    shell_init();
    print_serial("[M7] Interactive Kernel Shell ready.\n");

    // 11. Enable Hardware Interrupts
    asm volatile("sti");
    print_serial("[boot] Interrupts enabled (STI). Boot sequence finished!\n\n");

    // Run the shell task
    shell_task();

    // Fallback halt
    for (;;) {
        asm volatile("hlt");
    }
}
