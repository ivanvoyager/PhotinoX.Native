# In Visual Studio Developer PowerShell:
# tools\build-linux.ps1
# PhotinoX.Native\tools\build-linux.ps1
# or
# cd tools
#  .\build-linux.ps1
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Resolve-Path (Join-Path $ScriptDir "..")
Set-Location $ProjectRoot

Write-Host "Building in: $ProjectRoot" -ForegroundColor Cyan

docker run --rm -it -v ${PWD}:/src -w /src photinox-native-linux make build-photino-linux-x64

if ($LASTEXITCODE -eq 0) {
    Write-Host "Build successful!" -ForegroundColor Green
} else {
    Write-Host "Build failed!" -ForegroundColor Red
}