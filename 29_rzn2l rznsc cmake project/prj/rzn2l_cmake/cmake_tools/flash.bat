@echo off
setlocal

set "ROOT=%~dp0.."
for %%F in ("%ROOT%") do set "ROOT=%%~fF"
set "BUILD_DIR=%ROOT%\build_gcc"
set "SREC="
set "JLINK=%ProgramFiles%\SEGGER\JLink_V960\JLink.exe"

if not "%~1"=="" set "SREC=%~1"
if "%SREC%"=="" for /f "usebackq delims=" %%F in (`powershell -NoProfile -Command "$m = [regex]::Match((Get-Content -LiteralPath '%ROOT%\CMakeLists.txt' -Raw), '(?m)^\s*project\(\s*([^\s\)]+)'); if ($m.Success) { Write-Output (Join-Path '%BUILD_DIR%' ($m.Groups[1].Value + '.srec')) }"`) do set "SREC=%%F"

echo Current firmware target:
echo   %SREC%

if "%SREC%"=="" (
    echo Could not determine the project name from: %ROOT%\CMakeLists.txt
    exit /b 1
)

if not exist "%SREC%" (
    echo S-record file not found: %SREC%
    echo Please build the project first by running:
    echo   .\cmake_tools\build.bat
    exit /b 1
)

if not exist "%JLINK%" (
    echo J-Link was not found: %JLINK%
    exit /b 1
)
if not exist "%~dp0flash.jlink" (
    echo J-Link command file not found: %~dp0flash.jlink
    exit /b 1
)

copy /y "%~dp0flash.jlink" "%TEMP%\flash.jlink" >nul
if errorlevel 1 (
    echo Failed to prepare J-Link command file.
    exit /b 1
)
powershell -NoProfile -Command "$p = '%TEMP%\flash.jlink'; $s = Get-Content -LiteralPath $p -Raw; $s = $s.Replace('__SREC__', '"%SREC%"'); Set-Content -LiteralPath $p -Value $s -Encoding ASCII"
echo J-Link command:
findstr /I "loadfile" "%TEMP%\flash.jlink"
"%JLINK%" -device R9A07G084M04 -if swd -speed 4000 -CommanderScript "%TEMP%\flash.jlink"
exit /b %errorlevel%
