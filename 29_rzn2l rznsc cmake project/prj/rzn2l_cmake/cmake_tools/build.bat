@echo off
setlocal

set "ROOT=%~dp0.."
for %%F in ("%ROOT%") do set "ROOT=%%~fF"
set "BUILD_DIR=%ROOT%\build_gcc"

cd /d "%ROOT%"

if not exist "%BUILD_DIR%" (
    echo Initializing build directory...
    cmake -P "%ROOT%\cmake\prebuild.cmake"
    if errorlevel 1 exit /b 1
    call "%~dp0setup_cmake.bat"
    if errorlevel 1 exit /b 1
    cmake -S "%ROOT%" -B "%BUILD_DIR%" -G "MinGW Makefiles" -DCMAKE_TOOLCHAIN_FILE="%ROOT%/cmake/gcc.cmake"
    if errorlevel 1 exit /b 1
)

if exist "%BUILD_DIR%" (
    call "%~dp0setup_cmake.bat"
    if errorlevel 1 exit /b 1
)

cmake --build "%BUILD_DIR%"
if errorlevel 1 (
    echo.
    echo Build failed.
    exit /b 1
)

echo.
echo Build succeeded.
exit /b 0
