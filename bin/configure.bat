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
rem BUILD_TYPE: Release or Debug
cd "%BUILD_DIR%"
cmake -G "Visual Studio 15 2017 Win64" -D "CMAKE_PREFIX_PATH=%PREFIX_PATH%" -D "CMAKE_BUILD_TYPE=%BUILD_TYPE%" -D "CMAKE_INSTALL_PREFIX=%INSTALL_PREFIX%" %PROJECT_DIR%

@REM echo ___________________________________________________________________
@REM rem Enable Clang Tidy for Visual Studio 2019 IDE ...
@REM set ROBO_PROJ_FILE=%BUILD_DIR%/src/robomongo/robomongo.vcxproj
@REM python %BIN_DIR%\enable-visual-studio-clang-tidy.py %ROBO_PROJ_FILE% %BIN_DIR%