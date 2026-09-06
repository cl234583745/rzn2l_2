@echo off
setlocal

set "ROOT=%~dp0.."
for %%F in ("%ROOT%") do set "ROOT=%%~fF"
set "BUILD_DIR=%ROOT%\build_gcc"

if exist "%BUILD_DIR%" rmdir /s /q "%BUILD_DIR%"

cd /d "%ROOT%"
cmake -P "%ROOT%\cmake\prebuild.cmake"
if errorlevel 1 exit /b 1
call "%~dp0setup_cmake.bat"
if errorlevel 1 exit /b 1
cmake -S "%ROOT%" -B "%BUILD_DIR%" -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE="%ROOT%/cmake/gcc.cmake"
if errorlevel 1 exit /b 1
cmake --build "%BUILD_DIR%"
if errorlevel 1 (
    echo.
    echo Full rebuild failed.
    exit /b 1
)

echo.
echo Full rebuild succeeded.
exit /b 0
