#pragma once
#include <stdint.h>

// ============================================================
// Kernel Panic — NovaOS
//
// kernel_panic() is the last resort when the kernel encounters
// an unrecoverable condition. It:
//   1. Prints the panic message to the serial console.
//   2. Draws a solid red screen on the framebuffer.
//   3. Halts the CPU permanently (no return).
//
// Also exposes set_panic_framebuffer() so kernel_main can
// register the framebuffer pointer before the IDT is live.
// ============================================================

// Register the framebuffer with the panic system so it can
// draw a red screen on fault. Called from kernel_main.cpp
// after Limine provides the framebuffer response.
void set_panic_framebuffer(void* fb_addr, uint32_t width, uint32_t height, uint32_t pitch);

// Halt the system with an error message.
// Marked [[noreturn]] — this function never returns.
[[noreturn]] void kernel_panic(const char* message);
