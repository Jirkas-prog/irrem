param(
    [string]$Port,
    [switch]$Flash,
    [switch]$FirstInstall
)
$ErrorActionPreference = 'Stop'
if ($FirstInstall -and -not $Flash) { throw '-FirstInstall requires -Flash.' }
if ($Flash -and -not $Port) { throw 'Specify the new board explicitly with -Port COMx.' }
$projectPath = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$pioPath = Join-Path $env:USERPROFILE '.platformio\penv\Scripts\platformio.exe'
if (-not (Test-Path -LiteralPath $pioPath)) { $pioPath = (Get-Command pio -ErrorAction Stop).Source }
& $pioPath run --project-dir $projectPath -e tdisplay
if ($LASTEXITCODE -ne 0) { throw 'Firmware build failed.' }
if ($Flash) {
    if ($FirstInstall) {
        Write-Host "First installation: erasing the explicitly selected board on $Port."
        & $pioPath run --project-dir $projectPath -e tdisplay --target erase --upload-port $Port
        if ($LASTEXITCODE -ne 0) { throw 'Initial erase failed.' }
    }
    & $pioPath run --project-dir $projectPath -e tdisplay --target upload --upload-port $Port
    if ($LASTEXITCODE -ne 0) { throw 'Upload failed.' }
}
