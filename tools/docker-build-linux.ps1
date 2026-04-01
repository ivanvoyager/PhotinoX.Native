# In Visual Studio Developer PowerShell:
# tools\build-linux.ps1
# PhotinoX.Native\tools\build-linux.ps1
# or
# cd tools
#  .\build-linux.ps1

$ErrorActionPreference = "Stop"

$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path

$ProjectRoot = Resolve-Path (Join-Path $ScriptDir "..")

Write-Host "Project root: $ProjectRoot"

$ImageName = "photinox-native-linux"

Write-Host "Building Docker image '$ImageName'..."
docker build -t $ImageName $ProjectRoot

Write-Host "Docker image build completed successfully."