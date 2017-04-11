@echo off 
setlocal enableextensions enabledelayedexpansion

rem Path to bin and project folder
set BIN_DIR_WITH_BACKSLASH=%~dp0%
set BIN_DIR=%BIN_DIR_WITH_BACKSLASH:~0,-1%
set PROJECT_DIR=%BIN_DIR%\..

rem Run common setup code
call "%BIN_DIR%\common\setup.bat" %*
if %ERRORLEVEL% neq 0 (exit /b 1)

rem Run CMake configuration step
cd "%BUILD_DIR%"
cmake -G "Visual Studio 14 2015 Win64" -D "CMAKE_PREFIX_PATH=%PREFIX_PATH%" -D "CMAKE_BUILD_TYPE=%BUILD_TYPE%" -D "CMAKE_INSTALL_PREFIX=%INSTALL_PREFIX%" %PROJECT_DIR%