@echo off 
setlocal enableextensions enabledelayedexpansion

rem -----------------------------------
rem - Configuration
rem -----------------------------------
rem remove target folder
rm -rf build
if %ERRORLEVEL% neq 0 (
  echo.
  echo Error when removing !TARGET!.
  exit /b 1
  pause
)