@echo off
setlocal

set "ROOT=%~dp0.."
for %%F in ("%ROOT%") do set "ROOT=%%~fF"

powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0setup_cmake.ps1" -Root "%ROOT%"
if errorlevel 1 (
    echo.
    echo CMake project setup failed.
    exit /b 1
)

exit /b 0
