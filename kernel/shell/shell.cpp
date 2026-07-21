#include "shell.hpp"
#include "../drivers/framebuffer.hpp"
#include "../drivers/keyboard.hpp"
#include "../drivers/pit.hpp"
#include "../mm/pmm.hpp"
#include "../mm/heap.hpp"
#include "../sched/scheduler.hpp"
#include "../fs/vfs.hpp"
#include "../lib/panic.hpp"

extern "C" void print_serial(const char* str);

static bool streql(const char* a, const char* b) {
    while (*a && *b) {
        if (*a != *b) return false;
        ++a; ++b;
    }
    return *a == *b;
}

static bool strstarts(const char* str, const char* prefix) {
    while (*prefix) {
        if (*str != *prefix) return false;
        ++str; ++prefix;
    }
    return true;
}

static void print_both(const char* s) {
    fb_print(s);
    print_serial(s);
}

void shell_execute(const char* cmdline) {
    // Skip leading spaces
    while (*cmdline == ' ') ++cmdline;
    if (*cmdline == '\0') return;

    if (streql(cmdline, "help")) {
        fb_set_color(COLOR_CYAN, COLOR_DARK);
        print_both("Available NovaOS Shell Commands:\n");
        fb_set_color(COLOR_WHITE, COLOR_DARK);
        print_both("  help       - Display this assistance list\n");
        print_both("  clear      - Clear the screen console\n");
        print_both("  meminfo    - Show physical memory & heap statistics\n");
        print_both("  ticks      - Show system PIT timer tick count\n");
        print_both("  tasks      - List active kernel tasks\n");
        print_both("  ls         - List files in virtual filesystem\n");
        print_both("  cat <file> - Display contents of a file\n");
        print_both("  echo <txt> - Print text to console\n");
        print_both("  reboot     - Reset the computer\n");
        print_both("  panic      - Test kernel panic red screen\n");
    }
    else if (streql(cmdline, "clear")) {
        fb_clear();
        fb_set_color(COLOR_GREEN, COLOR_DARK);
        print_both("=== NovaOS Interactive Shell ===\n");
        fb_set_color(COLOR_WHITE, COLOR_DARK);
    }
    else if (streql(cmdline, "meminfo")) {
        fb_set_color(COLOR_YELLOW, COLOR_DARK);
        fb_print_dec("PMM Free Pages : ", pmm_free_pages());
        fb_print_dec("PMM Total Pages: ", pmm_total_pages());
        fb_print_dec("PMM Used (KB)  : ", pmm_used_bytes() / 1024);
        fb_set_color(COLOR_WHITE, COLOR_DARK);
    }
    else if (streql(cmdline, "ticks")) {
        fb_set_color(COLOR_YELLOW, COLOR_DARK);
        fb_print_dec("Timer Ticks: ", pit_ticks());
        fb_set_color(COLOR_WHITE, COLOR_DARK);
    }
    else if (streql(cmdline, "tasks")) {
        fb_set_color(COLOR_YELLOW, COLOR_DARK);
        fb_print_dec("Active Tasks: ", sched_task_count());
        fb_print_dec("Current Task ID: ", sched_current_id());
        fb_set_color(COLOR_WHITE, COLOR_DARK);
    }
    else if (streql(cmdline, "ls")) {
        fb_set_color(COLOR_CYAN, COLOR_DARK);
        print_both("RAM Disk Files:\n");
        const VFSNode* nodes[16];
        size_t n = vfs_list(nodes, 16);
        fb_set_color(COLOR_WHITE, COLOR_DARK);
        for (size_t i = 0; i < n; ++i) {
            print_both("  ");
            print_both(nodes[i]->name);
            fb_print_dec("  (", nodes[i]->size);
            print_both(" bytes)\n");
        }
    }
    else if (strstarts(cmdline, "cat ")) {
        const char* fname = cmdline + 4;
        while (*fname == ' ') ++fname;
        const VFSNode* node = vfs_open(fname);
        if (node) {
            fb_set_color(COLOR_GREEN, COLOR_DARK);
            print_both(node->data);
            if (node->data[node->size - 1] != '\n') print_both("\n");
            fb_set_color(COLOR_WHITE, COLOR_DARK);
        } else {
            fb_set_color(COLOR_RED, COLOR_DARK);
            print_both("File not found: ");
            print_both(fname);
            print_both("\n");
            fb_set_color(COLOR_WHITE, COLOR_DARK);
        }
    }
    else if (strstarts(cmdline, "echo ")) {
        print_both(cmdline + 5);
        print_both("\n");
    }
    else if (streql(cmdline, "reboot")) {
        print_both("Rebooting system...\n");
        // Pulse CPU reset via 8042 PS/2 controller port 0x64
        asm volatile(
            "mov $0xFE, %%al\n\t"
            "out %%al, $0x64\n\t"
            : : : "al"
        );
        for (;;) asm volatile("hlt");
    }
    else if (streql(cmdline, "panic")) {
        kernel_panic("Explicit user-triggered kernel panic via shell command!");
    }
    else {
        fb_set_color(COLOR_RED, COLOR_DARK);
        print_both("Unknown command: '");
        print_both(cmdline);
        print_both("'. Type 'help' for commands.\n");
        fb_set_color(COLOR_WHITE, COLOR_DARK);
    }
}

void shell_init() {
    fb_set_color(COLOR_GREEN, COLOR_DARK);
    print_both("\n=== NovaOS Interactive Shell ===\n");
    fb_set_color(COLOR_WHITE, COLOR_DARK);
    print_both("Type 'help' to see available commands.\n\n");
}

void shell_task() {
    static char buf[128];
    static size_t idx = 0;

    fb_set_color(COLOR_CYAN, COLOR_DARK);
    print_both("NovaOS> ");
    fb_set_color(COLOR_WHITE, COLOR_DARK);

    for (;;) {
        if (keyboard_has_char()) {
            char c = keyboard_getchar();
            if (c == '\n') {
                fb_putchar('\n');
                buf[idx] = '\0';
                shell_execute(buf);
                idx = 0;
                fb_set_color(COLOR_CYAN, COLOR_DARK);
                print_both("NovaOS> ");
                fb_set_color(COLOR_WHITE, COLOR_DARK);
            }
            else if (c == '\b') {
                if (idx > 0) {
                    --idx;
                    fb_putchar('\b');
                }
            }
            else if (c >= 32 && c <= 126) {
                if (idx < sizeof(buf) - 1) {
                    buf[idx++] = c;
                    fb_putchar(c);
                }
            }
        }
        // Yield execution to allow other tasks & IRQs to run
        asm volatile("hlt");
    }
}
