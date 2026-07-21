cmake_minimum_required(VERSION 3.20)

# Define system parameters before the project() call to configure cross-compilation.
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# Disable linking checks during compiler verification (avoids host ABI leakage checks).
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Enable both C++ and NASM assembly languages.
project(NovaOS LANGUAGES CXX ASM_NASM)

# ============================================================
# C++ Toolchain Settings
# ============================================================

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Compilation flags for a freestanding bare-metal x86-64 kernel.
set(KERNEL_COMPILE_FLAGS
    -target x86_64-elf        # Cross-compile for bare-metal ELF
    -ffreestanding            # No implicit stdlib includes
    -fno-exceptions           # No C++ exception runtime
    -fno-rtti                 # No runtime type info
    -mno-red-zone             # Kernel must not use the red zone (interrupt safety)
    -mno-mmx                  # No MMX (we don't save/restore FPU state yet)
    -mno-sse                  # No SSE
    -mno-sse2                 # No SSE2
    -mcmodel=kernel           # Use kernel code model: 64-bit RIP-relative addressing
                              # Required for a higher-half kernel at 0xFFFFFFFF80000000
    -Wall
    -Wextra
    -O2
)

# ============================================================
# NASM Settings
# ============================================================

set(CMAKE_ASM_NASM_COMPILE_OBJECT "<CMAKE_ASM_NASM_COMPILER> <FLAGS> -f elf64 -o <OBJECT> <SOURCE>")
set(CMAKE_ASM_NASM_FLAGS "-g -F dwarf")

# ============================================================
# Source Files
# ============================================================

set(KERNEL_SOURCES
    # Kernel entry point
    kernel/kernel_main.cpp

    # Milestone 2: CPU descriptor tables
    kernel/arch/x86_64/gdt.cpp
    kernel/arch/x86_64/idt.cpp
    kernel/arch/x86_64/isr_stubs.asm

    # Milestone 3: Memory Management
    kernel/mm/pmm.cpp
    kernel/mm/heap.cpp

    # Milestone 4: Hardware Drivers
    kernel/drivers/pic.cpp
    kernel/drivers/pit.cpp
    kernel/drivers/keyboard.cpp
    kernel/drivers/framebuffer.cpp

    # Milestone 5: Multitasking Scheduler
    kernel/sched/scheduler.cpp
    kernel/sched/context_switch.asm

    # Milestone 6: Virtual Filesystem
    kernel/fs/vfs.cpp

    # Milestone 7: Interactive Shell
    kernel/shell/shell.cpp

    # Kernel utilities
    kernel/lib/panic.cpp
)

# ============================================================
# Build Target
# ============================================================

add_executable(kernel.elf ${KERNEL_SOURCES})

target_compile_options(kernel.elf PRIVATE
    $<$<COMPILE_LANGUAGE:CXX>:${KERNEL_COMPILE_FLAGS}>
)

target_include_directories(kernel.elf PRIVATE
    kernel
    kernel/arch/x86_64
    kernel/lib
    kernel/mm
    kernel/drivers
    kernel/sched
    kernel/fs
    kernel/shell
)

set(CMAKE_CXX_LINK_EXECUTABLE "<CMAKE_LINKER> <LINK_FLAGS> <OBJECTS> -o <TARGET>")

target_link_options(kernel.elf PRIVATE
    -T ${CMAKE_SOURCE_DIR}/kernel/linker.ld
)

set_target_properties(kernel.elf PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    OUTPUT_NAME "kernel.elf"
)
