# Flask Project Manager Installer
# Auto-request administrator privileges

# Check if running as administrator, if not, relaunch with admin rights
if (-NOT ([Security.Principal.WindowsPrincipal] [Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
    Write-Host "Requesting administrator privileges..." -ForegroundColor Yellow
    Start-Process PowerShell -Verb RunAs "-NoProfile -ExecutionPolicy Bypass -File `"$PSCommandPath`""
    exit
}

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "    Flask Project Manager Installer" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

$InstallDir = "${env:ProgramFiles}\FlaskProjectManager"
$ExeUrl = "https://raw.githubusercontent.com/ToleInventor/flaskmanager/main/flask_manager.exe"
$ExePath = "$InstallDir\flask_manager.exe"

# Check if already installed
if (Test-Path $ExePath) {
    Write-Host "Flask Manager is already installed." -ForegroundColor Yellow
    Write-Host "Location: $ExePath" -ForegroundColor Gray
    Write-Host ""
    $reinstall = Read-Host "Reinstall? (y/n)"
    if ($reinstall -ne "y") {
        Write-Host "Installation cancelled." -ForegroundColor Yellow
        Read-Host "Press Enter to exit"
        exit 0
    }
    Write-Host ""
}

# Step 1: Create directory
Write-Host "[1/4] Creating installation directory..." -ForegroundColor Cyan
if (-not (Test-Path $InstallDir)) {
    New-Item -ItemType Directory -Path $InstallDir -Force | Out-Null
}
Write-Host "        Done." -ForegroundColor Green
Write-Host ""

# Step 2: Download file
Write-Host "[2/4] Downloading flask_manager.exe..." -ForegroundColor Cyan
try {
    Invoke-WebRequest -Uri $ExeUrl -OutFile $ExePath -UseBasicParsing
} catch {
    Write-Host "        ERROR: Download failed. Check internet connection." -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}
if (-not (Test-Path $ExePath)) {
    Write-Host "        ERROR: Download failed." -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}
Write-Host "        Done." -ForegroundColor Green
Write-Host ""

# Step 3: Add to System PATH
Write-Host "[3/4] Adding to system PATH..." -ForegroundColor Cyan
$CurrentPath = [Environment]::GetEnvironmentVariable("Path", "Machine")
if ($CurrentPath -notlike "*$InstallDir*") {
    $NewPath = $CurrentPath + ";" + $InstallDir
    [Environment]::SetEnvironmentVariable("Path", $NewPath, "Machine")
    Write-Host "        Added to system PATH." -ForegroundColor Green
} else {
    Write-Host "        Already in system PATH." -ForegroundColor Gray
}
Write-Host ""

# Step 4: Verify
Write-Host "[4/4] Verifying installation..." -ForegroundColor Cyan
if (Test-Path $ExePath) {
    Write-Host "        Verified: $ExePath" -ForegroundColor Green
} else {
    Write-Host "        ERROR: Installation file missing." -ForegroundColor Red
    Read-Host "Press Enter to exit"
    exit 1
}
Write-Host ""

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "    INSTALLATION COMPLETE" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Flask Manager has been installed to:" -ForegroundColor White
Write-Host "  $ExePath" -ForegroundColor Gray
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "    HOW TO RUN" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "METHOD 1 - Windows Search:" -ForegroundColor White
Write-Host "   Press Windows key, type 'Flask Manager', press Enter" -ForegroundColor Gray
Write-Host ""
Write-Host "METHOD 2 - Command Prompt:" -ForegroundColor White
Write-Host "   Close ALL Command Prompt windows, open a NEW one, then type: flask_manager" -ForegroundColor Gray
Write-Host ""
Write-Host "METHOD 3 - Run directly:" -ForegroundColor White
Write-Host "   `"$ExePath`"" -ForegroundColor Gray
Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "    TROUBLESHOOTING" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "If 'flask_manager' is not recognized in Command Prompt:" -ForegroundColor Yellow
Write-Host "   1. RESTART your computer (this forces PATH refresh)" -ForegroundColor Gray
Write-Host "   2. Or use METHOD 1 or METHOD 3 above" -ForegroundColor Gray
Write-Host ""
Read-Host "Press Enter to exit"
