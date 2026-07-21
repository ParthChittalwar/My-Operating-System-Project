# NovaOS Build Script
# Automates path discovery for Clang/Ninja/CMake/NASM, compiles the kernel, and prepares the ESP directory structure.

$ErrorActionPreference = "Stop"

$ProjectRoot = Split-Path -Parent $MyInvocation.MyCommand.Path
# Resolve to absolute path
$ProjectRoot = [System.IO.Path]::GetFullPath((Join-Path $ProjectRoot ".."))

Write-Output "=== NovaOS Build System ==="
Write-Output "Project Root: $ProjectRoot"

# 1. Path Discovery for Tools
Write-Output "Locating build tools..."

# Find Clang/LLVM-MinGW (installed via winget)
$ClangPath = Get-ChildItem -Path "$env:LOCALAPPDATA\Microsoft\WinGet\Packages" -Filter "clang++.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName -First 1
if (-not $ClangPath) {
    # Check if in PATH
    $ClangPath = Get-Command "clang++.exe" -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
}

# Find Ninja (installed via winget)
$NinjaPath = Get-ChildItem -Path "$env:LOCALAPPDATA\Microsoft\WinGet\Packages" -Filter "ninja.exe" -Recurse -ErrorAction SilentlyContinue | Select-Object -ExpandProperty FullName -First 1
if (-not $NinjaPath) {
    $NinjaPath = Get-Command "ninja.exe" -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
}

# Find Local CMake
$LocalCmakePath = Join-Path $ProjectRoot "toolchain\local\cmake\bin\cmake.exe"
if (-not (Test-Path $LocalCmakePath)) {
    $LocalCmakePath = Get-Command "cmake.exe" -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
}

# Find Local NASM
$LocalNasmPath = Join-Path $ProjectRoot "toolchain\local\nasm\nasm.exe"
if (-not (Test-Path $LocalNasmPath)) {
    $LocalNasmPath = Get-Command "nasm.exe" -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
}

# Verify all tools are found
if (-not $ClangPath) { throw "clang++.exe not found! Please check LLVM installation." }
if (-not $NinjaPath) { throw "ninja.exe not found! Please check Ninja installation." }
if (-not $LocalCmakePath) { throw "cmake.exe not found! Please check CMake installation." }
if (-not $LocalNasmPath) { throw "nasm.exe not found! Please check NASM installation." }

Write-Output "Found Clang: $ClangPath"
Write-Output "Found Ninja: $NinjaPath"
Write-Output "Found CMake: $LocalCmakePath"
Write-Output "Found NASM:  $LocalNasmPath"

# Add tool directories to PATH environment variable temporarily for the build process
$ClangDir = Split-Path $ClangPath
$NinjaDir = Split-Path $NinjaPath
$CmakeDir = Split-Path $LocalCmakePath
$NasmDir = Split-Path $LocalNasmPath

$env:PATH = "$ClangDir;$NinjaDir;$CmakeDir;$NasmDir;" + $env:PATH

# 2. Compile the Kernel
Write-Output "Configuring CMake..."
$BuildDir = Join-Path $ProjectRoot "build"
if (-not (Test-Path $BuildDir)) {
    New-Item -ItemType Directory -Path $BuildDir | Out-Null
}

# Configure build
& cmake.exe -B $BuildDir -G "Ninja" -DCMAKE_CXX_COMPILER="clang++" $ProjectRoot

# Compile build
Write-Output "Compiling Kernel..."
& cmake.exe --build $BuildDir

# 3. Prepare EFI System Partition (ESP) Directory Structure
Write-Output "Preparing EFI System Partition directory structure..."
$EspDir = Join-Path $BuildDir "esp"
$EfiBootDir = Join-Path $EspDir "EFI\BOOT"
$BootDir = Join-Path $EspDir "boot"

# Create directories
if (-not (Test-Path $EfiBootDir)) { New-Item -ItemType Directory -Path $EfiBootDir | Out-Null }
if (-not (Test-Path $BootDir)) { New-Item -ItemType Directory -Path $BootDir | Out-Null }

# Copy Limine bootloader BOOTX64.EFI
$LimineEfiSrc = Join-Path $ProjectRoot "third_party\limine\BOOTX64.EFI"
if (-not (Test-Path $LimineEfiSrc)) {
    throw "Limine BOOTX64.EFI not found! Please ensure third_party/limine submodule/clone is set up."
}
Copy-Item -Path $LimineEfiSrc -Destination (Join-Path $EfiBootDir "BOOTX64.EFI") -Force

# Copy Limine config
$LimineCfgSrc = Join-Path $ProjectRoot "boot\limine.conf"
Copy-Item -Path $LimineCfgSrc -Destination (Join-Path $EfiBootDir "limine.conf") -Force
# Copy to root/other folders as backup searches for Limine
Copy-Item -Path $LimineCfgSrc -Destination (Join-Path $EspDir "limine.conf") -Force

# Copy Kernel ELF binary
$KernelElfSrc = Join-Path $BuildDir "bin\kernel.elf"
if (-not (Test-Path $KernelElfSrc)) {
    throw "kernel.elf not found! Compilation might have failed."
}
Copy-Item -Path $KernelElfSrc -Destination (Join-Path $BootDir "kernel.elf") -Force

Write-Output "=== Build Succeeded ==="
Write-Output "ESP folder ready at: $EspDir"
