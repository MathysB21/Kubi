# Build the motion replay harness and run it over every recording in sim/motion.
# Expected results are in sim/motion/README.md.
$ErrorActionPreference = "Stop"
$sim = Split-Path -Parent $MyInvocation.MyCommand.Definition
$fw = Split-Path -Parent $sim
$exe = "$sim\replay_motion.exe"
& clang++ -std=c++17 -O1 -I "$sim\include" -I "$fw\include" -I "$fw\src" "$sim\src\replay_motion.cpp" "$fw\src\SensorManager.cpp" -static -lws2_32 -o $exe
if ($LASTEXITCODE -ne 0) { throw "replay_motion build failed" }
$files = Get-ChildItem "$sim\motion\*.csv" | ForEach-Object { $_.FullName }
& $exe @files | Where-Object { $_ -notmatch '^\[' }
