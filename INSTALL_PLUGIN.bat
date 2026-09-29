@echo off
setlocal
set "PLUGIN=%~dp0Acoustic Guitar by JGK.vst3"
set "DEST=%CommonProgramFiles%\VST3\Acoustic Guitar by JGK.vst3"

if not exist "%PLUGIN%" (
    echo ERROR: Acoustic Guitar by JGK.vst3 is not next to this installer.
    pause
    exit /b 1
)

net session >nul 2>&1
if not %errorlevel%==0 (
    powershell -NoProfile -ExecutionPolicy Bypass -Command "Start-Process -FilePath '%~f0' -Verb RunAs"
    exit /b
)

if exist "%DEST%" rmdir /S /Q "%DEST%"
xcopy "%PLUGIN%" "%DEST%\" /E /I /H /Y >nul

if errorlevel 1 (
    echo.
    echo Install failed. Try right-clicking INSTALL_PLUGIN.bat and choose Run as administrator.
    pause
    exit /b 1
)

echo.
echo Acoustic Guitar by JGK V5 is installed.
echo.
echo In FL Studio:
echo   Options ^> Manage plugins ^> Find installed plugins
echo.
pause
