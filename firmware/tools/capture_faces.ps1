# Face calibration for FaceMap.h. Walks through the four resting positions,
# averages the cube's accelerometer at each (GET /api/diagnostics over WiFi)
# and prints FACE_POSES and SCREEN_OUT_* ready to paste.
#
#   powershell -ExecutionPolicy Bypass -File firmware/tools/capture_faces.ps1 -Cube 192.168.10.36
#
# Redo it whenever the sensor moves relative to the screen (e.g. going from
# the breadboard into the wooden cube).
param(
    [string]$Cube = "kubi.local",
    [int]$Samples = 25
)
$ErrorActionPreference = "Stop"
# Decimal points in the output whatever the Windows locale (it is pasted into C++)
[System.Threading.Thread]::CurrentThread.CurrentCulture = [System.Globalization.CultureInfo]::InvariantCulture
$url = "http://$Cube/api/diagnostics"
$axisNames = @("X", "Y", "Z")

function Read-Pose([string]$title, [string]$how) {
    while ($true) {
        Write-Host ""
        Write-Host $title -ForegroundColor Cyan
        Write-Host "  $how"
        Write-Host "  Rest it there, let go if you can, then press Enter." -ForegroundColor DarkGray
        [void](Read-Host)
        $xs = @(); $ys = @(); $zs = @()
        for ($i = 0; $i -lt $Samples; $i++) {
            $d = Invoke-RestMethod -TimeoutSec 3 $url
            $xs += [double]$d.accelX; $ys += [double]$d.accelY; $zs += [double]$d.accelZ
            Start-Sleep -Milliseconds 100
        }
        $m = @(($xs | Measure-Object -Average).Average, ($ys | Measure-Object -Average).Average, ($zs | Measure-Object -Average).Average)
        $spread = 0.0
        foreach ($set in @($xs, $ys, $zs)) {
            $r = ($set | Measure-Object -Maximum -Minimum); $spread = [math]::Max($spread, $r.Maximum - $r.Minimum)
        }
        $mag = [math]::Sqrt($m[0] * $m[0] + $m[1] * $m[1] + $m[2] * $m[2])
        $abs = @([math]::Abs($m[0]), [math]::Abs($m[1]), [math]::Abs($m[2]))
        $axis = [array]::IndexOf($abs, ($abs | Measure-Object -Maximum).Maximum)
        $sign = 1; if ($m[$axis] -lt 0) { $sign = -1 }
        $share = $abs[$axis] / [math]::Max($mag, 0.001)
        Write-Host ("  x={0,6:N2}  y={1,6:N2}  z={2,6:N2}   |g|={3:N2}  spread={4:N2}  -> {5}{6} ({7:P0} of g)" -f $m[0], $m[1], $m[2], $mag, $spread, $(if ($sign -gt 0) { "+" } else { "-" }), $axisNames[$axis], $share)
        $problems = @()
        if ($spread -gt 1.5) { $problems += "it moved during the capture" }
        # ADXL345 zero-g offsets reach ~0.25 g per axis, so |g| of 8-11 is normal
        if ($mag -lt 7.0 -or $mag -gt 12.6) { $problems += "|g| is far from 9.8 (loose sensor or bad reads?)" }
        if ($share -lt 0.9) { $problems += "it is not resting square on one edge" }
        if ($problems.Count -eq 0) { return [pscustomobject]@{ Mean = $m; Axis = $axis; Sign = $sign } }
        Write-Host ("  Retake: " + ($problems -join "; ")) -ForegroundColor Yellow
    }
}

Write-Host "Kubi face calibration ($url)" -ForegroundColor Green
Write-Host "Keep the screen facing you the whole time and roll it like a steering wheel."
Write-Host "The display may switch faces while you do this; ignore it."
try { [void](Invoke-RestMethod -TimeoutSec 5 $url) } catch { throw "Cannot reach $url. Is the cube on WiFi? Try -Cube <its IP>." }

$poses = @(
    (Read-Pose "1/4  Face 1 (Clock)" "Screen facing you, panel upright in portrait: the way the boot screen text reads."),
    (Read-Pose "2/4  Face 2 (Pomodoro)" "From Face 1, roll a quarter turn to the LEFT (counter-clockwise: the top edge goes left)."),
    (Read-Pose "3/4  Face 3 (Mascot)" "Another quarter turn LEFT: now upside down compared with Face 1."),
    (Read-Pose "4/4  Face 4 (Ambient)" "Another quarter turn LEFT.")
)

# The four positions must use two axes, in opposite pairs (1/3 and 2/4)
$ok = $true
$inPlane = @($poses | ForEach-Object { $_.Axis } | Sort-Object -Unique)
if ($inPlane.Count -ne 2) { $ok = $false; Write-Host "Expected the four positions to use exactly two axes, got: $(($inPlane | ForEach-Object { $axisNames[$_] }) -join ', ')" -ForegroundColor Red }
if ($poses[0].Axis -ne $poses[2].Axis -or $poses[0].Sign -ne -$poses[2].Sign) { $ok = $false; Write-Host "Face 1 and Face 3 should be opposite on one axis." -ForegroundColor Red }
if ($poses[1].Axis -ne $poses[3].Axis -or $poses[1].Sign -ne -$poses[3].Sign) { $ok = $false; Write-Host "Face 2 and Face 4 should be opposite on one axis." -ForegroundColor Red }
if (-not $ok) { Write-Host "Check that the screen faced you throughout and each roll was a quarter turn, then run it again." -ForegroundColor Red; exit 1 }

# Screen normal (towards you) = up(Face 2) x up(Face 1) for a counter-clockwise roll
$u1 = @(0, 0, 0); $u1[$poses[0].Axis] = $poses[0].Sign
$u2 = @(0, 0, 0); $u2[$poses[1].Axis] = $poses[1].Sign
$out = @(($u2[1] * $u1[2] - $u2[2] * $u1[1]), ($u2[2] * $u1[0] - $u2[0] * $u1[2]), ($u2[0] * $u1[1] - $u2[1] * $u1[0]))
$outAxis = 0; for ($i = 0; $i -lt 3; $i++) { if ($out[$i] -ne 0) { $outAxis = $i } }
$outSign = $out[$outAxis]

$labels = @("Clock", "Pomodoro", "Mascot", "Ambient")
$date = Get-Date -Format "yyyy-MM-dd"
Write-Host ""
Write-Host "Paste into firmware/src/FaceMap.h:" -ForegroundColor Green
Write-Host "static const FacePose FACE_POSES[] = {"
for ($f = 0; $f -lt 4; $f++) {
    $p = $poses[$f]
    $s = "+1"; if ($p.Sign -lt 0) { $s = "-1" }
    $raw = "({0:N1}, {1:N1}, {2:N1})" -f $p.Mean[0], $p.Mean[1], $p.Mean[2]
    Write-Host ("    {{ AXIS_{0}, {1}, {2} }},  // Face {3}: {4,-9} measured {5} {6}" -f $axisNames[$p.Axis], $s, $f, ($f + 1), $labels[$f], $raw, $date)
}
Write-Host "};"
$os = "+1"; if ($outSign -lt 0) { $os = "-1" }
Write-Host ("static const GravityAxis SCREEN_OUT_AXIS = AXIS_{0};" -f $axisNames[$outAxis])
Write-Host ("static const int8_t      SCREEN_OUT_SIGN = {0};" -f $os)
