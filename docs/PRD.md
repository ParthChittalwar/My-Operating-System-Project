# Product Requirements Document
## Project Codename: **NovaOS**
### A Hobby x86-64 UEFI Operating System Written in C++ and Assembly

**Document Version:** 1.0
**Author:** Parth Chittalwar
**Status:** Draft — Living Document
**Last Updated:** July 2026

---

## 1. Vision

NovaOS is a learning-driven, from-scratch operating system built to demystify how modern computers boot, manage memory, schedule processes, and talk to hardware. The project treats "understanding" as the primary deliverable and a "working OS" as the proof of that understanding.

Unlike toy bootloader tutorials that stop at "Hello World in real mode," NovaOS targets a **modern, UEFI-based, 64-bit, modular kernel** — the same class of boot path used by real-world systems like Linux, Windows, and BSD today. The project is built entirely on free, open tooling (WSL, GCC/Clang, QEMU) so it's reproducible by anyone with a Windows machine, and it's structured to be **GitHub-ready** from day one: clean commit history, documented milestones, and a folder layout that reads like a serious systems project rather than a scratchpad.

**Guiding philosophy:**
> Build the smallest thing that boots, understand every line of it, then grow it one subsystem at a time.

---

## 2. Goals

### Primary Goals
1. Boot a custom 64-bit kernel via **UEFI** (no legacy BIOS/MBR path) on real x86-64 hardware and in QEMU.
2. Write the majority of the kernel in **C++** (freestanding, no STL/runtime dependency) with **Assembly** reserved for what only Assembly can do (context switches, interrupt stubs, low-level CPU instructions).
3. Build entirely on **Windows via WSL2**, using a Linux-native toolchain cross-compiled for a bare-metal target.
4. Design the kernel with **modular subsystems** (boot, memory, interrupts, drivers, filesystem, scheduler, shell) that can be developed, tested, and swapped independently.
5. Make the project **approachable to beginners** in OS development — heavily commented code, a documented build pipeline, and incremental milestones that always produce a bootable, demonstrable result.
6. Maintain a clean, professional **GitHub repository** suitable for a portfolio: README, architecture docs, CI build checks, and tagged releases per milestone.

### Non-Goals (for v1)
- POSIX compliance or running unmodified Linux binaries.
- SMP (multi-core) support in early milestones.
- GUI/windowing system (a simple text-mode or framebuffer console is sufficient initially).
- Networking stack (deferred to roadmap).
- Security hardening / production-grade robustness — this is a learning OS, not a shipped product.

---

## 3. Features

### 3.1 Boot & Firmware
- UEFI boot via a custom `.efi` bootloader (or GNU-EFI / Limine as a bootstrap, with a path to a fully custom bootloader later).
- GOP (Graphics Output Protocol) framebuffer acquisition for early graphics output.
- Memory map retrieval from UEFI before exiting boot services.
- Clean handoff from UEFI environment to kernel (`ExitBootServices`).

### 3.2 Core Kernel
- Long mode (64-bit) initialization: GDT, paging (4-level), IDT.
- Physical Memory Manager (PMM) — bitmap or buddy allocator.
- Virtual Memory Manager (VMM) — paging structures, kernel heap (`kmalloc`/`kfree`).
- Interrupt handling: ISRs, IRQs, PIC/APIC initialization.
- Timer (PIT/APIC timer) for preemptive scheduling ticks.
- Basic exception handling (page faults, GPFs) with diagnostic output.

### 3.3 Drivers
- Serial (UART) driver for debug logging (QEMU `-serial stdio`).
- Framebuffer/console driver (text rendering over GOP buffer).
- PS/2 keyboard driver.
- (Later) PCI enumeration, AHCI/NVMe disk driver, RTC.

### 3.4 Process & Scheduling
- Kernel-level task structure (TCB).
- Context switching (Assembly).
- Simple round-robin preemptive scheduler.
- Basic system call interface (`syscall`/`int 0x80` style).
- User mode / ring 3 support (later milestone).

### 3.5 Filesystem
- In-memory RAM disk / initrd for early milestones.
- VFS (Virtual File System) abstraction layer.
- FAT32 read support (since UEFI ESPs are FAT32 — natural fit).
- (Later) simple custom or ext2-like filesystem.

### 3.6 Shell / User Interface
- Minimal kernel shell (built-in commands: `help`, `clear`, `meminfo`, `ls`, `echo`, `reboot`).
- Command parser and dispatcher.
- Foundation for loading and running simple user-space programs.

### 3.7 Developer Experience
- One-command build via `make` / `ninja` / shell script.
- QEMU launch script with sane defaults (`-M q35 -cpu qemu64 -serial stdio -m 256M`).
- Debug support via QEMU + GDB stub.
- Modular CMake (or Makefile) so each subsystem compiles as its own object/library.
- Unit-testable components where feasible (e.g., allocator logic tested in userland harness).

---

## 4. Folder Structure

```
NovaOS/
├── README.md
├── LICENSE
├── .gitignore
├── docs/
│   ├── PRD.md                      # this document
│   ├── architecture.md
│   ├── build-guide.md
│   ├── milestones.md
│   └── diagrams/
...
```
*(Refer to README or source repository for full directory layout details).*
