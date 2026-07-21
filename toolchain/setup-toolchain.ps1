# NovaOS Toolchain Setup Script for Windows (Native Portable Setup)
# Downloads and extracts CMake and NASM locally inside the toolchain directory.
# This avoids the need for administrative privileges and keeps your system clean.

$ErrorActionPreference = "Stop"

# Define directories
$ToolchainDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$TargetDir = Join-Path $ToolchainDir "local"
if (-not (Test-Path $TargetDir)) {
    New-Item -ItemType Directory -Path $TargetDir | Out-Null
}

Write-Output "=== NovaOS Portable Toolchain Setup ==="
Write-Output "Target directory: $TargetDir"

# 1. Download and extract NASM
$NasmUrl = "https://www.nasm.us/pub/nasm/releasebuilds/2.16.03/win64/nasm-2.16.03-win64.zip"
$NasmZip = Join-Path $TargetDir "nasm.zip"
$NasmDest = Join-Path $TargetDir "nasm"

if (-not (Test-Path (Join-Path $NasmDest "nasm.exe"))) {
    Write-Output "Downloading NASM 2.16.03..."
    Invoke-WebRequest -Uri $NasmUrl -OutFile $NasmZip
    Write-Output "Extracting NASM..."
    Expand-Archive -Path $NasmZip -DestinationPath $TargetDir
    # Move extracted directory to 'nasm'
    $ExtractedFolder = Get-ChildItem $TargetDir -Directory | Where-Object { $_.Name -like "nasm-*" } | Select-Object -First 1
    if ($ExtractedFolder) {
        Rename-Item -Path $ExtractedFolder.FullName -NewName "nasm"
    }
    Remove-Item $NasmZip -Force
    Write-Output "NASM installed successfully."
} else {
    Write-Output "NASM is already installed."
}

# 2. Download and extract CMake
$CmakeUrl = "https://github.com/Kitware/CMake/releases/download/v4.3.4/cmake-4.3.4-windows-x86_64.zip"
$CmakeZip = Join-Path $TargetDir "cmake.zip"
$CmakeDest = Join-Path $TargetDir "cmake"

if (-not (Test-Path (Join-Path $CmakeDest "bin\cmake.exe"))) {
    Write-Output "Downloading CMake 4.3.4..."
    Invoke-WebRequest -Uri $CmakeUrl -OutFile $CmakeZip
    Write-Output "Extracting CMake..."
    Expand-Archive -Path $CmakeZip -DestinationPath $TargetDir
    # Move extracted directory to 'cmake'
    $ExtractedFolder = Get-ChildItem $TargetDir -Directory | Where-Object { $_.Name -like "cmake-*" } | Select-Object -First 1
    if ($ExtractedFolder) {
        Rename-Item -Path $ExtractedFolder.FullName -NewName "cmake"
    }
    Remove-Item $CmakeZip -Force
    Write-Output "CMake installed successfully."
} else {
    Write-Output "CMake is already installed."
}

Write-Output "=== Setup Completed ==="
Write-Output "Tools are located in:"
Write-Output "  - NASM: (Join-Path $NasmDest 'nasm.exe')"
Write-Output "  - CMake: (Join-Path $CmakeDest 'bin\cmake.exe')"
