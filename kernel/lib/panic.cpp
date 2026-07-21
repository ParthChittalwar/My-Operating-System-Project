#include "panic.hpp"

// ============================================================
// Kernel Panic Implementation — NovaOS
// ============================================================

// Framebuffer state registered by kernel_main before IDT goes live.
static void*    panic_fb_addr   = nullptr;
static uint32_t panic_fb_width  = 0;
static uint32_t panic_fb_height = 0;
static uint32_t panic_fb_pitch  = 0;

// Forward declaration: serial print from kernel_main.cpp.
// In a later milestone this moves to a proper serial driver module.
extern "C" void print_serial(const char* str);

void set_panic_framebuffer(void* fb_addr, uint32_t width, uint32_t height, uint32_t pitch) {
    panic_fb_addr   = fb_addr;
    panic_fb_width  = width;
    panic_fb_height = height;
    panic_fb_pitch  = pitch;
}

// Fill the entire framebuffer with a solid color.
// Used to paint the "red screen of death" on a kernel panic.
static void fill_screen(uint32_t color) {
    if (!panic_fb_addr) return;

    uint32_t* fb      = (uint32_t*)panic_fb_addr;
    uint32_t  pitch_p = panic_fb_pitch / 4; // pitch in pixels (assumes 32bpp)

    for (uint32_t y = 0; y < panic_fb_height; ++y) {
        for (uint32_t x = 0; x < panic_fb_width; ++x) {
            fb[y * pitch_p + x] = color;
        }
    }
}

[[noreturn]] void kernel_panic(const char* message) {
    // 1. Disable interrupts — we must not be preempted during panic.
    //    (Not strictly necessary in M2 since we have no timer IRQs,
    //     but it's the correct habit from the start.)
    asm volatile("cli");

    // 2. Print to serial so QEMU -serial stdio captures it.
    print_serial("\n\n");
    print_serial("*******************************\n");
    print_serial("*       KERNEL PANIC          *\n");
    print_serial("*******************************\n");
    print_serial(message);
    print_serial("\n");
    print_serial("System Halted. Please restart.\n");

    // 3. Paint the framebuffer bright red as a visual indicator.
    //    0x00CC0000 = solid red in XRGB8888 format.
    fill_screen(0x00CC0000);

    // 4. Halt the CPU forever.
    //    We use a loop with `hlt` — `hlt` suspends the CPU until
    //    the next interrupt. Since we disabled interrupts (cli),
    //    this is effectively an infinite spin with zero power draw.
    for (;;) {
        asm volatile("hlt");
    }
}
