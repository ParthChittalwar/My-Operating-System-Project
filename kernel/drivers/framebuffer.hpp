#pragma once
#include <stdint.h>
#include <stddef.h>

// ============================================================
// Framebuffer Text Console — M4/M7
//
// Renders text using an embedded 8×8 bitmap font on top of the
// Limine-provided GOP framebuffer.  Supports:
//   - Coloured foreground / background
//   - Scrolling when the bottom of screen is reached
//   - Cursor blinking (future)
// ============================================================

// Initialise the text console.
// fb_addr  : virtual address of the framebuffer (from Limine)
// width/height : pixels
// pitch    : bytes per scanline
void fb_init(void* fb_addr, uint32_t width, uint32_t height, uint32_t pitch);

// Set foreground / background colours (XRGB888).
void fb_set_color(uint32_t fg, uint32_t bg);

// Output a single character (handles '\n', '\r', '\b', '\t').
void fb_putchar(char c);

// Output a null-terminated string.
void fb_print(const char* s);

// Clear the screen and reset cursor to (0, 0).
void fb_clear();

// Print a formatted line with the current colours.
// (Minimal: no format string — just string + optional hex value)
void fb_print_hex(const char* prefix, uint64_t val);
void fb_print_dec(const char* prefix, uint64_t val);

// Current cursor row (useful for shell to restore position).
uint32_t fb_cursor_row();
uint32_t fb_cursor_col();

// Predefined colours.
static constexpr uint32_t COLOR_WHITE   = 0x00FFFFFF;
static constexpr uint32_t COLOR_BLACK   = 0x00000000;
static constexpr uint32_t COLOR_RED     = 0x00CC2200;
static constexpr uint32_t COLOR_GREEN   = 0x0022CC44;
static constexpr uint32_t COLOR_BLUE    = 0x002244CC;
static constexpr uint32_t COLOR_YELLOW  = 0x00CCCC00;
static constexpr uint32_t COLOR_CYAN    = 0x0000CCCC;
static constexpr uint32_t COLOR_MAGENTA = 0x00CC00CC;
static constexpr uint32_t COLOR_GRAY    = 0x00AAAAAA;
static constexpr uint32_t COLOR_DARK    = 0x00111122;
static constexpr uint32_t COLOR_ORANGE  = 0x00FF8C00;
