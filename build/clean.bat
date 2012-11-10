@echo off 
setlocal enableextensions enabledelayedexpansion

rem -----------------------------------
rem - Configuration
rem -----------------------------------

rem find project root folder
set PROJECT_ROOT=%CD%\..\

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

if %MODE% == all (
  call %0 debug
  call %0 release
  goto EOF
)

rem target folder where build will be placed
set TARGET=%PROJECT_ROOT%target\%MODE%

if not exist %TARGET% (
  echo.
  echo Already clean: !TARGET!.
  goto EOF
  pause
)

rem remove target folder
rmdir /s /q %TARGET%
if %ERRORLEVEL% neq 0 (
  echo.
  echo Error when removing !TARGET!.
  exit /b 1
  pause
)

rem check that clean was successfull
if not exist %TARGET% (
  echo.
  echo Done without errors.
  echo This folder removed: !TARGET!
) else (
  echo.
  echo Error when removing files. Not all files were removed.
)

:EOF