@echo off
setlocal

call "bin\setup_cl_x64.bat"

set BUILD_TYPE=Debug
set BUILD_DIR=cmake-build-debug
set CXX_FLAGS=/std:c++latest /ZI
set MOLE_BUILD_TESTS=ON

if "%1"=="-release" (
    set BUILD_TYPE=Release
    set BUILD_DIR=cmake-build-release
    set CXX_FLAGS=/std:c++latest
    set MOLE_BUILD_TESTS=OFF
)

if not exist "%BUILD_DIR%" (
    mkdir "%BUILD_DIR%"
)

cmake -S . -B "%BUILD_DIR%" -G Ninja -DCMAKE_BUILD_TYPE=%BUILD_TYPE% -DCMAKE_CXX_STANDARD=26 -DCMAKE_CXX_FLAGS="%CXX_FLAGS%" -DMOLE_BUILD_TESTS=%MOLE_BUILD_TESTS%
if errorlevel 1 (
    echo CMake configuration failed.
    exit /b 1
)

cmake --build "%BUILD_DIR%" --config %BUILD_TYPE% --verbose -j 14
if errorlevel 1 (
    echo Build failed.
    exit /b 1
)

echo Build succeeded in %BUILD_DIR% (%BUILD_TYPE%).
endlocal