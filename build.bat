@echo off
:: Yotobe One-Click Build Script
setlocal enabledelayedexpansion

echo ========================================================
echo               Building Yotobe Desktop App
echo ========================================================

:: 1. Locate Visual Studio MSVC environment
for /f "usebackq tokens=*" %%i in (`"%ProgramFiles(x86)%\Microsoft Visual Studio\Installer\vswhere.exe" -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath`) do (
    set "VS_DIR=%%i"
)

if not defined VS_DIR (
    echo [ERROR] Visual Studio C++ Build Tools not found!
    echo Please run install_vs_buildtools.bat first.
    pause
    exit /b 1
)

echo Found Visual Studio at: %VS_DIR%
call "%VS_DIR%\VC\Auxiliary\Build\vcvars64.bat"

:: 2. Set Qt 6.8 Path
set "QT_DIR=C:\Qt\6.8.2\msvc2022_64"
if not exist "%QT_DIR%" (
    echo [ERROR] Qt directory %QT_DIR% not found!
    pause
    exit /b 1
)

echo Found Qt at: %QT_DIR%

:: 3. Configure and Build with CMake and Ninja
cmake -B build -G "Ninja" -DCMAKE_PREFIX_PATH="%QT_DIR%" -DCMAKE_BUILD_TYPE=Release
if %errorlevel% neq 0 (
    echo [ERROR] CMake configuration failed!
    pause
    exit /b %errorlevel%
)

cmake --build build --config Release
if %errorlevel% neq 0 (
    echo [ERROR] Build failed!
    pause
    exit /b %errorlevel%
)

:: 4. Deploy Qt runtime dependencies using windeployqt
echo Deploying Qt WebEngine runtime libraries...
"%QT_DIR%\bin\windeployqt.exe" build\Yotobe.exe

:: Copy yt-dlp to build folder if not already there
if exist yt-dlp.exe (
    copy /y yt-dlp.exe build\yt-dlp.exe >nul
)

echo ========================================================
echo SUCCESS! Yotobe is ready at: build\Yotobe.exe
echo ========================================================
pause
