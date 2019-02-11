@echo off
setlocal enableextensions enabledelayedexpansion

rem Path to bin and project folder
set BIN_DIR_WITH_BACKSLASH=%~dp0%
set BIN_DIR=%BIN_DIR_WITH_BACKSLASH:~0,-1%
set PROJECT_DIR=%BIN_DIR%\..

rem Run tests
echo.
echo *******************  Running unit tests for Release mode ******************* 
echo ** Note: Only Release mode is supported on Windows 
echo.
echo "Running file: \build\Release\src\robomongo-unit-tests\Release\robo_unit_tests.exe"
call "%PROJECT_DIR%\\build\Release\src\robomongo-unit-tests\Release\robo_unit_tests.exe" %*