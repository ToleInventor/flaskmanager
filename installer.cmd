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
set "DESKTOP_SHORTCUT=%USERPROFILE%\Desktop\Flask Manager.lnk"
set "STARTMENU_SHORTCUT=%APPDATA%\Microsoft\Windows\Start Menu\Programs\Flask Manager.lnk"

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

echo [1/6] Creating installation directory...
mkdir "%INSTALL_DIR%" 2>nul
if not exist "%INSTALL_DIR%" (
    echo ERROR: Failed to create directory. Run as Administrator.
    pause
    exit /b 1
)
echo        Done.
echo.

echo [2/6] Downloading flask_manager.exe...
powershell -Command "& {[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri '%EXE_URL%' -OutFile '%EXE_PATH%'}" >nul 2>&1
if not exist "%EXE_PATH%" (
    echo ERROR: Download failed. Check internet connection.
    pause
    exit /b 1
)
echo        Done.
echo.

echo [3/6] Adding to system PATH...

:: Get current PATH from registry
for /f "skip=2 tokens=3*" %%a in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v Path 2^>nul') do set "CURRENT_PATH=%%a %%b"

:: Remove trailing space
set "CURRENT_PATH=!CURRENT_PATH: =!"

:: Check if already in PATH
echo !CURRENT_PATH! | find /i "%INSTALL_DIR%" >nul
if errorlevel 1 (
    powershell -Command "[Environment]::SetEnvironmentVariable('Path', [Environment]::GetEnvironmentVariable('Path', 'Machine') + ';%INSTALL_DIR%', 'Machine')" >nul 2>&1
    if !errorlevel! equ 0 (
        echo        Added to system PATH.
    ) else (
        echo        WARNING: Could not add to PATH.
    )
) else (
    echo        Already in system PATH.
)
echo.

echo [4/6] Creating desktop shortcut...
powershell -Command "$WshShell = New-Object -comObject WScript.Shell; $Shortcut = $WshShell.CreateShortcut('%DESKTOP_SHORTCUT%'); $Shortcut.TargetPath = '%EXE_PATH%'; $Shortcut.WorkingDirectory = '%INSTALL_DIR%'; $Shortcut.Description = 'Flask Project Manager'; $Shortcut.Save()" >nul 2>&1
if exist "%DESKTOP_SHORTCUT%" (
    echo        Desktop shortcut created.
) else (
    echo        WARNING: Could not create desktop shortcut.
)
echo.

echo [5/6] Creating Start Menu shortcut...
powershell -Command "$WshShell = New-Object -comObject WScript.Shell; $Shortcut = $WshShell.CreateShortcut('%STARTMENU_SHORTCUT%'); $Shortcut.TargetPath = '%EXE_PATH%'; $Shortcut.WorkingDirectory = '%INSTALL_DIR%'; $Shortcut.Description = 'Flask Project Manager'; $Shortcut.Save()" >nul 2>&1
if exist "%STARTMENU_SHORTCUT%" (
    echo        Start Menu shortcut created.
) else (
    echo        WARNING: Could not create Start Menu shortcut.
)
echo.

echo [6/6] Verifying installation...
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
echo METHOD 1 - Desktop shortcut (created for you)
echo   Double-click "Flask Manager" on your desktop
echo.
echo METHOD 2 - Start Menu
echo   Click Start and type "Flask Manager"
echo.
echo METHOD 3 - Command Prompt
echo   Close ALL Command Prompt windows, open a NEW one, then type: flask_manager
echo.
echo METHOD 4 - Run directly
echo   "%EXE_PATH%"
echo.
echo ========================================
echo    TROUBLESHOOTING
echo ========================================
echo.
echo If 'flask_manager' is not recognized in Command Prompt:
echo   1. Restart your computer (forces PATH refresh)
echo   2. Or use the desktop shortcut instead
echo.
pause
