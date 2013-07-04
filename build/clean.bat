@echo off 
setlocal enableextensions enabledelayedexpansion

rem -----------------------------------
rem - Configuration
rem -----------------------------------
set arch_bit=%1
if %1. ==. (
set arch_bit=32
)
rem remove target folder
rmdir build
if %ERRORLEVEL% neq 0 (
  echo.
  echo Error when removing !TARGET!.
  exit /b 1
  pause
)