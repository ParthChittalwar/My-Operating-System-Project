#pragma once
#include <stdint.h>
#include <stddef.h>

// ============================================================
// PS/2 Keyboard Driver — M4
//
// Reads scancodes from the PS/2 controller (port 0x60),
// converts set-1 scancodes to ASCII, and stores them in a
// circular ring buffer. Interrupts are driven by IRQ1.
// ============================================================

// Initialise the keyboard driver and enable IRQ1.
void keyboard_init();

// Returns true if there is at least one character in the buffer.
bool keyboard_has_char();

// Block until a character is available, then return it.
// Only returns printable ASCII, Enter ('\n'), or Backspace ('\b').
char keyboard_getchar();

// Non-blocking peek: return 0 if buffer empty.
char keyboard_peekchar();
