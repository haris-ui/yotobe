@echo off
setlocal
echo ===================================================
echo             Building Yotobe Mobile APK             
echo ===================================================

set "JAVA_HOME=C:\Program Files\Java\jdk-21.0.11"
set "GRADLE_BIN=C:\Users\HaRIS\.gradle\wrapper\dists\gradle-8.11.1-all\2qik7nd48slq1ooc2496ixf4i\gradle-8.11.1\bin\gradle.bat"

if not exist "%GRADLE_BIN%" (
    echo [ERROR] Gradle 8.11.1 binary not found at %GRADLE_BIN%
    pause
    exit /b 1
)

cd /d "%~dp0android"
call "%GRADLE_BIN%" assembleRelease --stacktrace

if %ERRORLEVEL% equ 0 (
    echo.
    echo ===================================================
    echo  APK Build Successful! Copying to dist and root...
    echo ===================================================
    cd /d "%~dp0"
    copy /Y "android\app\build\outputs\apk\release\app-release.apk" "Yotobe.apk"
    copy /Y "android\app\build\outputs\apk\release\app-release.apk" "dist\Yotobe.apk"
    echo Created: %~dp0Yotobe.apk
    echo Created: %~dp0dist\Yotobe.apk
    echo.
    echo Done! You can transfer Yotobe.apk to your phone or run: adb install -r Yotobe.apk
) else (
    echo.
    echo [ERROR] Build failed! Check Gradle output above.
)

pause
