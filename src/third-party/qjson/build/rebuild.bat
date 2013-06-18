@echo off 
setlocal enableextensions enabledelayedexpansion

rem -----------------------------------
rem - Configuration
rem -----------------------------------

rem mode is a first arg
set MODE=%1

rem if mode wasnt specified - debug mode assumed
if %1. ==. (
  set MODE=debug
)

rem if not 'debug' AND 'release' - print error message
if not %MODE% == debug (
  if not %MODE% == release (
    if not %MODE% == all (
      echo.
      echo Specified mode ^(%MODE%^) is unsupported.
      exit /b 1
    )
  )
)

call clean.bat %MODE%
call build.bat %MODE%