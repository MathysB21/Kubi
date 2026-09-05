# Kubi Desktop C++ Simulator Build Script
$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$firmwareDir = Split-Path -Parent $scriptDir

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  COMPILING KUBI C++ HARDWARE SIMULATOR   " -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan

# Find clang++
$clang = Get-Command "clang++.exe" -ErrorAction SilentlyContinue
if (-not $clang) {
    # Fallback to winget path
    $wingetClang = Get-ChildItem "C:\Users\Mathys\AppData\Local\Microsoft\WinGet\Packages" -Recurse -Filter "clang++.exe" -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($wingetClang) {
        $clang = $wingetClang.FullName
    } else {
        Write-Error "clang++.exe not found! Please ensure LLVM-MinGW is installed."
    }
} else {
    $clang = $clang.Source
}

Write-Host "Using compiler: $clang" -ForegroundColor Green

$srcFiles = @(
    "$firmwareDir\src\PomodoroManager.cpp",
    "$firmwareDir\src\DisplayManager.cpp",
    "$firmwareDir\src\SensorManager.cpp",
    "$firmwareDir\src\AudioManager.cpp",
    "$firmwareDir\src\KubiSprites.cpp",
    "$scriptDir\src\TFT_eSPI_Mock.cpp",
    "$scriptDir\src\sim_main.cpp"
)

$includes = @(
    "-I", "$scriptDir\include",
    "-I", "$firmwareDir\include",
    "-I", "$firmwareDir\src",
    "-I", "$firmwareDir\.pio\libdeps\esp32dev\ArduinoJson\src"
)

$outputExe = "$scriptDir\kubi_sim.exe"

$args = @(
    "-std=c++17",
    "-O2",
    "-static"
) + $includes + $srcFiles + @("-lws2_32", "-o", $outputExe)

Write-Host "Compiling actual firmware C++ sources into standalone $outputExe..." -ForegroundColor Yellow
& $clang $args

if ($LASTEXITCODE -eq 0) {
    Write-Host "[SUCCESS] kubi_sim.exe compiled successfully!" -ForegroundColor Green
} else {
    Write-Error "[ERROR] Compilation failed with exit code $LASTEXITCODE"
}
