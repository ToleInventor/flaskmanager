@echo off
title Flask Project Manager Installer
setlocal enabledelayedexpansion

:: Auto-elevate to Administrator
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Requesting administrator privileges...
    powershell start -verb runas '%0'
    exit /b
)

cls
echo ========================================
echo    Flask Project Manager Installer
echo ========================================
echo.

set "INSTALL_DIR=%ProgramFiles%\FlaskProjectManager"
set "EXE_URL=https://raw.githubusercontent.com/ToleInventor/flaskmanager/main/flask_manager.exe"
set "EXE_PATH=%INSTALL_DIR%\flask_manager.exe"

if exist "%EXE_PATH%" (
    echo Flask Manager is already installed.
    echo Location: %EXE_PATH%
    echo.
    set /p REINSTALL="Reinstall? (y/n): "
    if /i not "!REINSTALL!"=="y" (
        echo Installation cancelled.
        pause
        exit /b 0
    )
    echo.
)

echo [1/4] Creating installation directory...
mkdir "%INSTALL_DIR%" 2>nul
if not exist "%INSTALL_DIR%" (
    echo ERROR: Failed to create directory. Run as Administrator.
    pause
    exit /b 1
)
echo        Done.
echo.

echo [2/4] Downloading flask_manager.exe...
powershell -Command "& {[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri '%EXE_URL%' -OutFile '%EXE_PATH%'}" >nul 2>&1
if not exist "%EXE_PATH%" (
    echo ERROR: Download failed. Check internet connection.
    pause
    exit /b 1
)
echo        Done.
echo.

echo [3/4] Adding to system PATH...

:: Get current PATH from registry
for /f "skip=2 tokens=3*" %%a in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v Path 2^>nul') do set "CURRENT_PATH=%%a %%b"

:: Remove trailing space
set "CURRENT_PATH=!CURRENT_PATH: =!"

:: Check if already in PATH
echo !CURRENT_PATH! | find /i "%INSTALL_DIR%" >nul
if errorlevel 1 (
    :: Use PowerShell to add PATH (handles long paths correctly)
    powershell -Command "[Environment]::SetEnvironmentVariable('Path', [Environment]::GetEnvironmentVariable('Path', 'Machine') + ';%INSTALL_DIR%', 'Machine')" >nul 2>&1
    if %errorlevel% equ 0 (
        echo        Added to system PATH.
    ) else (
        echo        WARNING: Could not add to PATH via PowerShell.
        echo        Please add manually: %INSTALL_DIR%
    )
) else (
    echo        Already in system PATH.
)
echo.

echo [4/4] Verifying installation...
if exist "%EXE_PATH%" (
    echo        Verified: %EXE_PATH%
) else (
    echo        ERROR: Installation file missing.
    pause
    exit /b 1
)
echo.

echo ========================================
echo    INSTALLATION COMPLETE
echo ========================================
echo.
echo Flask Manager has been installed to:
echo   %INSTALL_DIR%
echo.
echo ========================================
echo    HOW TO RUN
echo ========================================
echo.
echo METHOD 1 - Command Prompt (after PATH is set):
echo   1. Close ALL Command Prompt windows
echo   2. Open a NEW Command Prompt
echo   3. Type: flask_manager
echo.
echo METHOD 2 - Run directly (always works):
echo   "%EXE_PATH%"
echo.
echo METHOD 3 - Create desktop shortcut:
echo   1. Right-click desktop
echo   2. New -> Shortcut
echo   3. Location: "%EXE_PATH%"
echo   4. Name: Flask Manager
echo.
echo ========================================
echo    TROUBLESHOOTING
echo ========================================
echo.
echo If 'flask_manager' is not recognized:
echo   1. Restart your computer (this forces PATH refresh)
echo   2. Or use METHOD 2 or 3 above
echo.
pause
