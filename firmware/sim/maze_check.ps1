# Build the maze board checker against the real MazeGame/MazeBoards sources
# and run it. See the header of sim/src/maze_check.cpp for what it checks.
# It runs from a temp directory: the Preferences mock writes kubi_sim_prefs.txt
# in the working directory, and the sim's own prefs must not be touched.
$ErrorActionPreference = "Stop"
$sim = Split-Path -Parent $MyInvocation.MyCommand.Definition
$fw = Split-Path -Parent $sim
$work = Join-Path $env:TEMP "kubi_maze_check"
New-Item -ItemType Directory -Force $work | Out-Null
$exe = Join-Path $work "maze_check.exe"
& clang++ -std=c++17 -O2 -I "$sim\include" -I "$fw\include" -I "$fw\src" "$sim\src\maze_check.cpp" "$fw\src\MazeGame.cpp" "$fw\src\MazeBoards.cpp" "$sim\src\TFT_eSPI_Mock.cpp" -static -lws2_32 -o $exe
if ($LASTEXITCODE -ne 0) { throw "maze_check build failed" }
Push-Location $work
try {
    Remove-Item -ErrorAction SilentlyContinue kubi_sim_prefs.txt
    & $exe @args   # e.g. -File maze_check.ps1 --show 12 prints board 12 with its route
    $code = $LASTEXITCODE
} finally {
    Pop-Location
}
exit $code
