# NovaOS Architecture Documentation

## 1. High-Level System Memory Layout

NovaOS uses a 64-bit higher-half kernel memory layout linked at virtual base `0xFFFFFFFF80000000`.

```
Virtual Address Space (64-bit)
┌─────────────────────────────────────────────────────────┐ 0xFFFFFFFFFFFFFFFF
│                      Kernel Space                        │
│ 0xFFFFFFFF80000000 -> Kernel ELF (.text, .rodata, .data) │
│ 0xFFFF800000000000 -> Limine HHDM (Direct Physical Map) │
├─────────────────────────────────────────────────────────┤
│                      Canonical Hole                     │
├─────────────────────────────────────────────────────────┤ 0x00007FFFFFFFFFFF
│                      User Space                         │ (Reserved for Ring 3)
└─────────────────────────────────────────────────────────┘ 0x0000000000000000
```

## 2. Boot & Firmware Sequence

1. **UEFI Firmware (QEMU OVMF)** initializes platform hardware.
2. **Limine Bootloader (`BOOTX64.EFI`)** reads `limine.conf`, loads `kernel.elf` into higher-half memory, sets up 4-level paging, and requests memory map & GOP framebuffer.
3. **`_start()` Entry**: Kernel initializes subsystems in deterministic sequence:
   - Serial UART debug port (`0x3F8`)
   - Global Descriptor Table (GDT) & Task State Segment (TSS)
   - Limine Framebuffer & Panic subsystem
   - Physical Memory Manager (PMM Bitmap)
   - Kernel Heap (`kmalloc`/`kfree`)
   - 8259A PIC Remap & PIT Timer (100 Hz)
   - PS/2 Keyboard Driver
   - Interrupt Descriptor Table (IDT with 32 exceptions + IRQs)
   - Virtual File System (VFS RAM Disk)
   - Round-Robin Preemptive Scheduler
   - Interactive Kernel Shell (`shell_task`)

## 3. Subsystem Architecture

### CPU Descriptor Tables (`gdt.cpp` & `idt.cpp`)
- GDT has 6 entries: Null, 64-bit Kernel Code (`0x08`), Kernel Data (`0x10`), User Code (`0x18`), User Data (`0x20`), and 16-byte TSS (`0x28`).
- Segment registers are reloaded via a far return (`lretq`) trick.
- IDT registers 256 gates (Type `0x8E` = 64-bit interrupt gate).
- Vectors 0–31 map to `isr_stubs.asm` trampolines.
- Vector 32 (IRQ0 / PIT) and Vector 33 (IRQ1 / Keyboard) dispatch hardware interrupts.

### Physical Memory Manager (`pmm.cpp`)
- Uses a 128 KB bit array (1,048,576 bits) covering up to 4 GB of RAM.
- `0` = in-use or reserved, `1` = usable physical page (4 KB).
- Physical address conversion uses Limine's HHDM offset: `phys_to_virt(p) = p + hhdm_offset`.

### Kernel Heap (`heap.cpp`)
- First-fit linked list allocator operating over physical pages mapped through HHDM.
- Supports `kmalloc(size)`, `kfree(ptr)`, and `kzalloc(size)`.

### Multitasking Scheduler (`scheduler.cpp` & `context_switch.asm`)
- Preemptive Round-Robin scheduler tracking up to 16 tasks via Task Control Blocks (TCB).
- Switch happens during PIT IRQ ticks every time-slice (2 ticks = 20ms).
- `context_switch(old_rsp*, new_rsp)` saves and restores callee-saved registers (`rbx, rbp, r12, r13, r14, r15`) on the task's kernel stack.
