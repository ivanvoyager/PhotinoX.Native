@echo off
setlocal enabledelayedexpansion

echo ==========================================
echo Photino Native Restore Script
echo Current Directory: %cd%
echo ==========================================
echo.

rem Define possible paths to the SAME project file
set "PROJ_PATH_A=.\Photino.Native\Photino.Native.vcxproj"
set "PROJ_PATH_B=.\PhotinoX.Native\Photino.Native\Photino.Native.vcxproj"

set "BUILD_PARAMS=/t:Restore /p:Configuration=Debug /p:Platform=x64"

set "DONE=false"

rem --- Attempt 1: Check standard path ---
if exist "%PROJ_PATH_A%" (
    echo [OK] Project found at: %PROJ_PATH_A%
    echo Running msbuild restore...
    msbuild "%PROJ_PATH_A%" %BUILD_PARAMS%
    if !errorlevel! neq 0 (
        echo [ERROR] Failed to restore project.
        goto :end
    )
    set "DONE=true"
    goto :finish
) else (
    echo [INFO] Path not found: %PROJ_PATH_A%
)

echo.

rem --- Attempt 2: Check alternative path ---
if exist "%PROJ_PATH_B%" (
    echo [OK] Project found at: %PROJ_PATH_B%
    echo Running msbuild restore...
    msbuild "%PROJ_PATH_B%" %BUILD_PARAMS%
    if !errorlevel! neq 0 (
        echo [ERROR] Failed to restore project.
        goto :end
    )
    set "DONE=true"
    goto :finish
) else (
    echo [INFO] Path not found: %PROJ_PATH_B%
)

if "%DONE%"=="false" (
    echo.
    echo ==========================================
    echo [WARNING] Project file not found in any expected location.
    echo Please ensure you are running this script from the solution root 
    echo or a valid subdirectory containing the native project.
    goto :end
)

:finish
echo.
echo ==========================================
echo Done. Project restored successfully.
echo ==========================================
:end
rem pause
