# NovaOS Build & Toolchain Guide

## Overview

NovaOS features a self-contained, portable Windows toolchain setup. It avoids WSL2 complexity by using **LLVM/Clang** cross-compilation (`-target x86_64-elf`), **NASM**, **CMake**, **Ninja**, and **QEMU + OVMF**.

## Prerequisites

On Windows 11, open PowerShell and install LLVM-MinGW and Ninja:

```powershell
winget install MartinStorsjo.LLVM-MinGW.UCRT
winget install Ninja-build.Ninja
```

The build script will automatically detect these tools or download local portable binaries for CMake, NASM, and QEMU inside `toolchain/local/`.

## Building the OS

To build the kernel executable (`bin/kernel.elf`) and assemble the EFI System Partition (`build/esp/`):

```powershell
.\scripts\build.ps1
```

### Build Artifacts Created:
- `build/bin/kernel.elf`: The 64-bit higher-half kernel binary.
- `build/esp/EFI/BOOT/BOOTX64.EFI`: The Limine UEFI bootloader loader.
- `build/esp/EFI/BOOT/limine.conf`: The Limine configuration file.
- `build/esp/boot/kernel.elf`: The kernel binary inside the virtual disk image.

## Running in QEMU

To test NovaOS inside QEMU with UEFI firmware:

```powershell
.\scripts\run-qemu.ps1
```

This launches:
```powershell
qemu-system-x86_64.exe -bios OVMF.fd -net none -drive file=fat:rw:build/esp,format=raw -serial stdio
```

## Troubleshooting

- **Compiler Error: `unknown target x86_64-elf`**: Make sure LLVM/Clang is installed, not standard MinGW GCC. Clang natively supports `-target x86_64-elf`.
- **Linker Error: `R_X86_64_32 out of range`**: Ensure `-mcmodel=kernel` is specified in `CMakeLists.txt` so Clang generates 64-bit RIP-relative addresses.
- **QEMU fails to boot**: Ensure `OVMF.fd` is present in the project root directory.
