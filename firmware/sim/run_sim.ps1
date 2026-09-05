# Kubi Hardware Simulation Studio Runner
$ErrorActionPreference = "Continue"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Definition
$projectDir = Split-Path -Parent (Split-Path -Parent $scriptDir)
$dashboardDir = "$projectDir\dashboard"
$exePath = "$scriptDir\kubi_sim.exe"

# 1. Kill any existing kubi_sim instances
Get-Process -Name "kubi_sim" -ErrorAction SilentlyContinue | Stop-Process -Force

# 2. Build kubi_sim.exe
Write-Host "Building latest kubi_sim.exe..." -ForegroundColor Cyan
& "$scriptDir\build_sim.ps1"
if ($LASTEXITCODE -ne 0) {
    Write-Error "Build failed. Exiting."
    exit 1
}

Write-Host "=================================================" -ForegroundColor Cyan
Write-Host "      LAUNCHING KUBI HARDWARE SIMULATION STUDIO  " -ForegroundColor Cyan
Write-Host "=================================================" -ForegroundColor Cyan

# 3. Start C++ Hardware Runner
Write-Host "[1/2] Starting Native C++ Firmware Runner on http://127.0.0.1:8080 ..." -ForegroundColor Green
$simProcess = Start-Process -FilePath $exePath -WorkingDirectory $scriptDir -PassThru -NoNewWindow

Start-Sleep -Seconds 1

# 4. Launch Vite Dashboard
Write-Host "[2/2] Starting React Companion Workbench..." -ForegroundColor Green
Write-Host ""
Write-Host ">>> OPEN YOUR BROWSER AT: http://localhost:5173/sim <<<" -ForegroundColor Yellow
Write-Host ">>> Press Ctrl+C in this terminal to shut down studio <<<" -ForegroundColor Yellow
Write-Host ""

try {
    Set-Location $dashboardDir
    & npm run dev
} finally {
    Write-Host "`nStopping C++ Firmware Runner..." -ForegroundColor Gray
    if ($simProcess -and -not $simProcess.HasExited) {
        Stop-Process -Id $simProcess.Id -Force -ErrorAction SilentlyContinue
    }
}
