@echo off
title Flask Project Manager
setlocal enabledelayedexpansion

:: Auto-elevate to Administrator
net session >nul 2>&1
if %errorlevel% neq 0 (
    echo Requesting administrator privileges...
    powershell start -verb runas '%0'
    exit /b
)

set "INSTALL_DIR=%ProgramFiles%\FlaskProjectManager"
set "EXE_URL=https://raw.githubusercontent.com/ToleInventor/flaskmanager/main/flask_manager.exe"
set "EXE_PATH=%INSTALL_DIR%\flask_manager.exe"

:START
cls
echo ========================================
echo    Flask Project Manager
echo ========================================
echo.

if exist "%EXE_PATH%" (
    echo [STATUS] Flask Manager is INSTALLED
    echo.
    echo Location: %EXE_PATH%
    echo.
    echo ========================================
    echo Choose an option:
    echo.
    echo   1. Launch Flask Manager
    echo   2. Uninstall Flask Manager
    echo   3. Exit
    echo.
    set /p CHOICE="Enter choice (1/2/3): "
    
    if "!CHOICE!"=="1" goto LAUNCH
    if "!CHOICE!"=="2" goto UNINSTALL
    if "!CHOICE!"=="3" goto EXIT
    goto START
) else (
    echo [STATUS] Flask Manager is NOT INSTALLED
    echo.
    echo Would you like to install it now?
    echo.
    set /p INSTALL_CHOICE="Install Flask Manager? (y/n): "
    if /i "!INSTALL_CHOICE!"=="y" goto INSTALL
    goto EXIT
)

:INSTALL
cls
echo ========================================
echo    Installing Flask Manager
echo ========================================
echo.

echo Creating installation directory...
mkdir "%INSTALL_DIR%" 2>nul

echo Downloading flask_manager.exe from GitHub...
powershell -Command "& {[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest -Uri '%EXE_URL%' -OutFile '%EXE_PATH%'}"

if not exist "%EXE_PATH%" (
    echo.
    echo Download failed. Please check your internet connection.
    pause
    goto EXIT
)

echo Download complete.
echo.

echo Adding to system PATH...
set "PATH_ENTRY=%INSTALL_DIR%"
for /f "skip=2 tokens=3*" %%a in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v Path 2^>nul') do set "CURRENT_PATH=%%a %%b"
echo "!CURRENT_PATH!" | find /i "!PATH_ENTRY!" >nul
if errorlevel 1 (
    setx /m Path "!CURRENT_PATH!;!PATH_ENTRY!" >nul
    echo Added to system PATH.
) else (
    echo Already in system PATH.
)

echo.
echo ========================================
echo Installation Successful!
echo ========================================
echo.
echo You can now run 'flask_manager' from any Command Prompt.
echo.
pause
goto LAUNCH

:UNINSTALL
cls
echo ========================================
echo    Uninstalling Flask Manager
echo ========================================
echo.

set /p CONFIRM="Are you sure you want to uninstall? (y/n): "
if /i not "!CONFIRM!"=="y" goto START

echo.
echo Removing from system PATH...

for /f "skip=2 tokens=3*" %%a in ('reg query "HKLM\SYSTEM\CurrentControlSet\Control\Session Manager\Environment" /v Path 2^>nul') do set "CURRENT_PATH=%%a %%b"
set "NEW_PATH=!CURRENT_PATH!;"
set "NEW_PATH=!NEW_PATH:%INSTALL_DIR%;=;!"
set "NEW_PATH=!NEW_PATH:%INSTALL_DIR%=!"
set "NEW_PATH=!NEW_PATH:;;=;!"
if "!NEW_PATH:~-1!"==";" set "NEW_PATH=!NEW_PATH:~0,-1!"
if "!NEW_PATH:~0,1!"==";" set "NEW_PATH=!NEW_PATH:~1!"

setx /m Path "!NEW_PATH!" >nul
echo Removed from system PATH.

echo Deleting installation directory...
rmdir /s /q "%INSTALL_DIR%" 2>nul

if not exist "%EXE_PATH%" (
    echo.
    echo ========================================
    echo Uninstall Successful!
    echo ========================================
) else (
    echo.
    echo Uninstall failed. Some files could not be deleted.
    echo Close any programs using flask_manager and try again.
)

echo.
pause
goto START

:LAUNCH
cls
echo ========================================
echo    Launching Flask Manager
echo ========================================
echo.

if not exist "%EXE_PATH%" (
    echo Error: flask_manager.exe not found.
    echo Please reinstall the application.
    pause
    goto START
)

echo Starting Flask Manager...
start "" "%EXE_PATH%"
echo.
echo Flask Manager is now running in a new window.
echo You can close this window.
echo.
pause
goto EXIT

:EXIT
exit /b