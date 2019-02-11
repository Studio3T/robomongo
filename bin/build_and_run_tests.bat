@echo off
setlocal enableextensions enabledelayedexpansion

rem Path to bin and project folder
set BIN_DIR_WITH_BACKSLASH=%~dp0%
set BIN_DIR=%BIN_DIR_WITH_BACKSLASH:~0,-1%
set PROJECT_DIR=%BIN_DIR%\..

rem Run build
call "%BIN_DIR%\build.bat" %*
if %ERRORLEVEL% neq 0 (exit /b 1)

rem Run tests
call "%BIN_DIR%\run_tests.bat" %*
if %ERRORLEVEL% neq 0 (exit /b 1)