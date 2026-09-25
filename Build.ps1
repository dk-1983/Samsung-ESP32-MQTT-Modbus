param([switch]$TestOnly)
$ErrorActionPreference = 'Stop'
Set-Location -LiteralPath $PSScriptRoot
$python = Join-Path $PSScriptRoot 'work/venv/Scripts/python.exe'
if (!(Test-Path -LiteralPath $python)) {
  py -3.11 -m venv work/venv
  if ($LASTEXITCODE) { throw 'Cannot create Python environment' }
}
& $python -m pip install -r requirements.txt
if ($LASTEXITCODE) { throw 'Cannot install build dependencies' }
& $python tests/run.py
if ($LASTEXITCODE) { throw 'Host tests failed' }
if ($TestOnly) { exit 0 }
if (!(Test-Path -LiteralPath 'secrets.yaml')) {
  & $python tools/setup_secrets.py
  if ($LASTEXITCODE) { throw 'Cannot generate local credentials' }
}
$env:PLATFORMIO_CORE_DIR = Join-Path $PSScriptRoot 'work/platformio'
& $python -m esphome compile samsung-s3.yaml
if ($LASTEXITCODE) { throw 'Firmware build failed' }
Write-Host 'Build only: no device was flashed. Local credentials are in secrets.yaml.'
