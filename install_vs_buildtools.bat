@echo off
:: Yotobe - Visual Studio 2022 Build Tools Installer
:: This script requests Administrator elevation and installs MSVC with C++ tools
:: using --nocache to minimize disk space consumption.

net session >nul 2>&1
if %errorLevel% neq 0 (
    echo Requesting Administrator privileges to install Visual Studio C++ Build Tools...
    powershell -Command "Start-Process cmd -ArgumentList '/c \"%~dpnx0\"' -Verb RunAs"
    exit /b
)

echo ========================================================
echo Installing Visual Studio 2022 C++ Build Tools...
echo Mode: --nocache (Saves ~3GB disk space)
echo ========================================================

set "TEMP_EXE=%LOCALAPPDATA%\Temp\WinGet\Microsoft.VisualStudio.2022.BuildTools.17.14.41\vs_BuildTools.exe"

if exist "%TEMP_EXE%" (
    echo Using downloaded installer: %TEMP_EXE%
    "%TEMP_EXE%" --passive --wait --norestart --nocache --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended
) else (
    echo Downloading installer...
    powershell -Command "Invoke-WebRequest -Uri 'https://aka.ms/vs/17/release/vs_BuildTools.exe' -OutFile '%TEMP%\vs_BuildTools.exe'"
    "%TEMP%\vs_BuildTools.exe" --passive --wait --norestart --nocache --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended
)

echo.
echo ========================================================
echo Installation completed! You can now build Yotobe.
echo ========================================================
pause
