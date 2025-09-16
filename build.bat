@echo off
setlocal

call "bin\setup_cl_x64.bat"

set BUILD_PRESET=debug
set BUILD_DIR=cmake-build-debug
set MOLE_BUILD_UI=ON

if "%1"=="-release" (
    set BUILD_PRESET=release
    set BUILD_DIR=cmake-build-release
) else if "%2"=="-release" (
    set BUILD_PRESET=release
    set BUILD_DIR=cmake-build-release
)

if "%1"=="-no-ui" (
    set BUILD_PRESET=%BUILD_PRESET%-no-ui
    set BUILD_DIR=%BUILD_DIR%-no-ui
    set MOLE_BUILD_UI=OFF
) else if "%2"=="-no-ui" (
    set BUILD_PRESET=%BUILD_PRESET%-no-ui
    set BUILD_DIR=%BUILD_DIR%-no-ui
    set MOLE_BUILD_UI=OFF
)

if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
)

cmake --preset %BUILD_PRESET%
if errorlevel 1 (
    echo CMake configuration failed.
    exit /b 1
)

cmake --build --preset %BUILD_PRESET%
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

echo Build succeeded in %BUILD_DIR% (%BUILD_PRESET%).
endlocal