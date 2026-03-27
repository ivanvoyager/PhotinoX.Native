# In Visual Studio Developer PowerShell:
# tools\restore-native.ps1
# or
# cd tools
#  .\restore-native.ps1

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

Write-Host "=========================================="
Write-Host " Photino Native Restore Script"
Write-Host " Script Directory: $PSScriptRoot"
Write-Host "=========================================="
Write-Host ""

# Compute the project path exactly as required
$proj = Join-Path (Join-Path $PSScriptRoot "..") "Photino.Native\Photino.Native.vcxproj"

Write-Host "Checking project path:"
Write-Host "  $proj"
Write-Host ""

if (-not (Test-Path $proj)) {
    Write-Host "[ERROR] Project not found." -ForegroundColor Red
    exit 1
}

Write-Host "[OK] Project found."
Write-Host "Running msbuild restore..."
Write-Host ""

$buildParams = @('/t:Restore','/p:Configuration=Debug','/p:Platform=x64')
& msbuild $proj @buildParams

if ($LASTEXITCODE -ne 0) {
    Write-Host "[ERROR] Restore failed." -ForegroundColor Red
    exit 1
}

Write-Host ""
Write-Host "=========================================="
Write-Host " Done. Project restored successfully."
Write-Host "=========================================="