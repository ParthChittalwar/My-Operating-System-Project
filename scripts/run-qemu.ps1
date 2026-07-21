# NovaOS Run Script
# Compiles the kernel and boots it in QEMU using UEFI firmware (OVMF.fd) and a virtual FAT drive.

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = [System.IO.Path]::GetFullPath((Join-Path $ProjectRoot ".."))

Write-Output "=== Launching NovaOS in QEMU ==="

# 1. Build the project first
& powershell -ExecutionPolicy Bypass -File (Join-Path $ProjectRoot "scripts\build.ps1")

# 2. Locate QEMU executable
Write-Output "Locating QEMU..."
    # Check local installation folder
    $LocalPath = Join-Path $ProjectRoot "toolchain\local\qemu\qemu-system-x86_64.exe"
    if (Test-Path $LocalPath) {
        $QemuPath = $LocalPath
    }
    if (-not $QemuPath) {
        # Check standard installation folder
        $StandardPath = "C:\Program Files\qemu\qemu-system-x86_64.exe"
        if (Test-Path $StandardPath) {
            $QemuPath = $StandardPath
        }
    }

if (-not $QemuPath) {
    throw "qemu-system-x86_64.exe not found! Please check QEMU installation."
}
Write-Output "Found QEMU: $QemuPath"

# Verify OVMF.fd is present
$OvmfPath = Join-Path $ProjectRoot "OVMF.fd"
if (-not (Test-Path $OvmfPath)) {
    throw "OVMF.fd not found in project root! Please run build or check downloads."
}

# Define the build ESP directory
$BuildDir = Join-Path $ProjectRoot "build"
$EspDir = Join-Path $BuildDir "esp"

# Launch QEMU with UEFI bios, disabled network, serial output redirected to console, and virtual FAT drive
Write-Output "Booting QEMU..."
& $QemuPath -bios $OvmfPath -net none -drive "file=fat:rw:$EspDir,format=raw" -serial stdio
