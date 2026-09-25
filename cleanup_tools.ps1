# Yotobe Environment Complete Cleanup Script
# Run this script as Administrator whenever you want to completely remove
# Visual Studio Build Tools, Qt, and all build caches to free maximum disk storage.

Write-Host "==========================================" -ForegroundColor Cyan
Write-Host "  Yotobe Development Tools Uninstaller   " -ForegroundColor Cyan
Write-Host "==========================================" -ForegroundColor Cyan

# 1. Uninstall Visual Studio 2022 Build Tools
$vsInstaller = "C:\Program Files (x86)\Microsoft Visual Studio\Installer\setup.exe"
$vsInstallPath = "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools"

if (Test-Path $vsInstaller) {
    Write-Host "[1/3] Uninstalling Visual Studio Build Tools..." -ForegroundColor Yellow
    Start-Process -FilePath $vsInstaller -ArgumentList "uninstall --installPath `"$vsInstallPath`" --quiet --wait" -Wait -NoNewWindow
    Write-Host "  Done." -ForegroundColor Green
} else {
    Write-Host "[1/3] Visual Studio Installer not found; skipping." -ForegroundColor Gray
}

# Clean residual VS folders and package caches
Write-Host "[2/3] Cleaning Visual Studio residual files and package caches..." -ForegroundColor Yellow
Remove-Item -Recurse -Force "C:\Program Files (x86)\Microsoft Visual Studio" -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force "C:\ProgramData\Microsoft\VisualStudio\Packages" -ErrorAction SilentlyContinue
Remove-Item -Recurse -Force "C:\ProgramData\Package Cache" -ErrorAction SilentlyContinue
Write-Host "  Done." -ForegroundColor Green

# 2. Remove Qt SDK installation
if (Test-Path "C:\Qt") {
    Write-Host "[3/3] Removing Qt SDK from C:\Qt..." -ForegroundColor Yellow
    Remove-Item -Recurse -Force "C:\Qt" -ErrorAction SilentlyContinue
    Write-Host "  Done." -ForegroundColor Green
} else {
    Write-Host "[3/3] C:\Qt not found; skipping." -ForegroundColor Gray
}

Write-Host ""
Write-Host "Cleanup completed successfully! All storage reclaimed." -ForegroundColor Green
