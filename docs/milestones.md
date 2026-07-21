# NovaOS Milestone History

| # | Milestone | Key Deliverables | Status |
|---|---|---|---|
| M0 | **Environment Setup** | LLVM-MinGW, CMake, Ninja, NASM, QEMU, OVMF configuration scripts | ✅ Complete |
| M1 | **UEFI Handoff** | Limine 8.x configuration, GOP framebuffer, serial UART logging | ✅ Complete |
| M2 | **Long Mode Core** | GDT/TSS, IDT with 32 CPU exceptions, NASM ISR trampolines, kernel panic | ✅ Complete |
| M3 | **Memory Management** | PMM bitmap allocator (4GB physical RAM), kernel heap (`kmalloc`/`kfree`) | ✅ Complete |
| M4 | **Hardware Drivers** | 8259A PIC, 100 Hz PIT timer, PS/2 keyboard, Framebuffer text console | ✅ Complete |
| M5 | **Multitasking** | TCB structures, NASM `context_switch`, Round-Robin scheduler | ✅ Complete |
| M6 | **Virtual Filesystem** | In-memory RAM Disk VFS (`sysinfo.txt`, `readme.txt`, `credits.txt`) | ✅ Complete |
| M7 | **Kernel Shell** | Interactive prompt with `help`, `meminfo`, `tasks`, `ls`, `cat`, `reboot`, `panic` | ✅ Complete |
