#include "keyboard.hpp"
#include "pic.hpp"

static constexpr uint16_t PS2_DATA = 0x60;

static inline uint8_t inb(uint16_t port) {
    uint8_t v; asm volatile("inb %1,%0":"=a"(v):"Nd"(port):"memory"); return v;
}

// Ring buffer
static constexpr size_t KB_BUFSIZE = 256;
static volatile char kb_buf[KB_BUFSIZE];
static volatile size_t kb_head = 0; // write index
static volatile size_t kb_tail = 0; // read index

static bool shift_held  = false;
static bool caps_locked = false;

// Scancode Set 1 → ASCII (unshifted)
static const char sc_to_ascii[128] = {
    0,   0,  '1','2','3','4','5','6','7','8','9','0','-','=', '\b', // 0x00–0x0E
    '\t','q','w','e','r','t','y','u','i','o','p','[',']', '\n',      // 0x0F–0x1C
    0,   'a','s','d','f','g','h','j','k','l',';','\'','`',           // 0x1D–0x29
    0,  '\\','z','x','c','v','b','n','m',',','.','/', 0,             // 0x2A–0x36
    '*', 0,  ' ', 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0, // 0x37–0x46
    '7','8','9','-','4','5','6','+','1','2','3','0','.',             // 0x47–0x53
    0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,        // fill
    0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,        // fill
    0,   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0  // fill
};

static const char sc_to_ascii_shift[128] = {
    0,   0,  '!','@','#','$','%','^','&','*','(',')','_','+', '\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}', '\n',
    0,   'A','S','D','F','G','H','J','K','L',':','"','~',
    0,  '|','Z','X','C','V','B','N','M','<','>','?', 0,
    '*', 0,  ' ', 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 0, 0,
    '7','8','9','-','4','5','6','+','1','2','3','0','.',
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

// Called by the IRQ1 dispatcher in idt.cpp.
extern "C" void keyboard_irq_handler() {
    uint8_t sc = inb(PS2_DATA);

    // Key release events have bit 7 set.
    if (sc & 0x80) {
        uint8_t base = sc & 0x7F;
        if (base == 0x2A || base == 0x36) shift_held = false; // L/R Shift release
        return;
    }

    // Press events
    if (sc == 0x2A || sc == 0x36) { shift_held = true;  return; } // Shift press
    if (sc == 0x3A) { caps_locked = !caps_locked;       return; } // Caps Lock toggle
    if (sc == 0x1D) return; // Left Ctrl  — ignored for now
    if (sc == 0x38) return; // Left Alt   — ignored for now

    char c = 0;
    if (sc < 128) {
        bool upper = (shift_held ^ caps_locked) &&
                     (sc_to_ascii[sc] >= 'a' && sc_to_ascii[sc] <= 'z');
        if (shift_held) c = sc_to_ascii_shift[sc];
        else if (upper)  c = (char)(sc_to_ascii[sc] - 32);
        else             c = sc_to_ascii[sc];
    }

    if (c) {
        size_t next = (kb_head + 1) % KB_BUFSIZE;
        if (next != kb_tail) { // drop if full
            kb_buf[kb_head] = c;
            kb_head = next;
        }
    }
}

void keyboard_init() {
    pic_enable_irq(1); // IRQ1 → vector 33
}

bool keyboard_has_char() { return kb_head != kb_tail; }

char keyboard_getchar() {
    while (!keyboard_has_char()) asm volatile("hlt");
    char c = kb_buf[kb_tail];
    kb_tail = (kb_tail + 1) % KB_BUFSIZE;
    return c;
}

char keyboard_peekchar() {
    if (!keyboard_has_char()) return 0;
    return kb_buf[kb_tail];
}
