# Reads the last crash from the cube's coredump partition over USB and prints
# the crashing task's backtrace with source lines, plus every task's state.
#
#   powershell -ExecutionPolicy Bypass -File firmware/tools/read_coredump.ps1 -Port COM5
#
# The ELF must be the build that crashed (a rebuild of the same commit works).
# Hold BOOT if the read stalls at "Connecting...". Needs esp-coredump in
# PlatformIO's Python: & "$env:USERPROFILE\.platformio\penv\Scripts\python.exe" -m pip install esp-coredump
param(
    [string]$Port = "COM5",
    [string]$Elf = "$PSScriptRoot\..\.pio\build\esp32dev\firmware.elf"
)
$ErrorActionPreference = "Stop"

$python = "$env:USERPROFILE\.platformio\penv\Scripts\python.exe"
$gdb = "$env:USERPROFILE\.platformio\packages\toolchain-xtensa-esp32\bin\xtensa-esp32-elf-gdb.exe"
if (-not (Test-Path $Elf)) { throw "No ELF at $Elf. Build the firmware that crashed first (pio run)." }
if (-not (Test-Path $gdb)) { throw "xtensa gdb not found at $gdb. Build the firmware once so PlatformIO installs the toolchain." }

# Keep the raw dump next to the build so it can be decoded again later
$saved = Join-Path (Split-Path $Elf) ("coredump-" + (Get-Date -Format "yyyyMMdd-HHmmss") + ".elf")

# 0x3F0000 is the coredump partition in partitions.csv
& $python -m esp_coredump --chip esp32 --port $Port info_corefile --off 0x3F0000 --gdb $gdb --save-core $saved $Elf
if ($LASTEXITCODE -eq 0) { Write-Host "Raw dump saved to $saved" -ForegroundColor Green }
