# Compile every example sketch in Examples/ against the local rpdsp and
# pico_audio_i2s libraries. Does NOT flash.
#
# Usage: pwsh scripts/build_all_examples.ps1
#        (or: powershell -File scripts/build_all_examples.ps1)
# Exit: non-zero if any example fails to compile.
#
# Requires: arduino-cli on PATH, rp2040:rp2040 core installed.
#
# NOTE: $ErrorActionPreference is deliberately left at default (Continue).
# Setting it to 'Stop' makes PowerShell abort on any line arduino-cli writes
# to stderr — but gcc prints benign warnings to stderr on every build, so
# that would halt the script after the first example. We rely on
# $LASTEXITCODE instead.

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$RepoRoot  = Split-Path -Parent $ScriptDir
$Fqbn      = (Get-Content "$ScriptDir\fqbn.txt" -Raw).Trim()
$ExamplesDir = Join-Path $RepoRoot "Examples"

if (-not (Get-Command arduino-cli -ErrorAction SilentlyContinue)) {
    Write-Error "arduino-cli not found on PATH. Install: https://arduino.github.io/arduino-cli/latest/"
    exit 1
}

# Discover examples: every subdirectory of Examples/ containing a .ino.
$examples = Get-ChildItem -Path $ExamplesDir -Directory | Where-Object {
    (Get-ChildItem -Path $_.FullName -Filter *.ino -File).Count -gt 0
}

if ($examples.Count -eq 0) {
    Write-Error "No examples found in $ExamplesDir"
    exit 1
}

Write-Host "Building $($examples.Count) examples with FQBN:"
Write-Host "  $Fqbn"
Write-Host ""

$pass = 0
$fail = 0
$failed = @()

foreach ($ex in $examples) {
    $name = $ex.Name
    $logFile = Join-Path $env:TEMP "build_$name.log"
    Write-Host -NoNewline ("{0,-32} ... " -f $name)

    # Redirect both stdout and stderr to the log; do NOT let stderr trip
    # ErrorActionPreference (arduino-cli/gcc prints benign warnings there).
    & arduino-cli compile `
        --fqbn $Fqbn `
        --library (Join-Path $RepoRoot "libraries\rpdsp") `
        --library (Join-Path $RepoRoot "libraries\pico_audio_i2s") `
        $ex.FullName *> $logFile

    if ($LASTEXITCODE -eq 0) {
        Write-Host "PASS"
        $pass++
    } else {
        Write-Host "FAIL (see $logFile)"
        $fail++
        $failed += $name
    }
}

Write-Host ""
Write-Host "Results: $pass passed, $fail failed (of $($pass + $fail))"

if ($fail -gt 0) {
    Write-Error "Failed: $($failed -join ', ')"
    exit 1
}
