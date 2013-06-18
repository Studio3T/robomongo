@echo off 
setlocal enableextensions enabledelayedexpansion

rem -----------------------------------
rem - Configuration
rem -----------------------------------

rem Visual C++ directory
set VISUALC_DIR=%ProgramFiles%\Microsoft Visual Studio 10.0\VC

rem store current directory in order to return to it after
set ORIGINAL_DIR=%CD%

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
      goto FINALLY
    )
  )
)

if %MODE% == all (
  %0 debug
  %0 release
  goto FINALLY
)

rem target folder where build will be placed
set TARGET=%PROJECT_ROOT%target\%MODE%

if %MODE% == debug (
  set CMAKE_ARGS= -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug
)

if %MODE% == release (
  set CMAKE_ARGS= -G"NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
)

echo Target folder: %TARGET%
echo Visual C folder: %VISUALC_DIR%

REM -----------------------------------
REM - Preparation
REM -----------------------------------

REM create target folder (if not already exists)
if not exist %TARGET% mkdir %TARGET%
if %ERRORLEVEL% neq 0 goto ERROR

REM go to target folder
cd %TARGET%
if %ERRORLEVEL% neq 0 goto ERROR

echo.

REM -----------------------------------
REM - Visual C Environment
REM -----------------------------------

REM configure Visual C shell environment
IF "!%VSINSTALLDIR%!" == "!!" (

  echo Settings Visual C environment variables...
  call "%VISUALC_DIR%\bin\vcvars32.bat"
  if %ERRORLEVEL% neq 0 goto ERROR
  
) else ( 
  echo Skipping Visual C environment setup ^(already configured^)
)

REM -----------------------------------
REM - QMake
REM -----------------------------------

REM run qmake
cmake %CMAKE_ARGS% %PROJECT_ROOT%src
if %ERRORLEVEL% neq 0 (
  echo.
  echo Error when running qmake.
  goto FINALLY
)

REM -----------------------------------
REM - Visual C nmake
REM -----------------------------------

nmake
if %ERRORLEVEL% neq 0 (
  echo.
  echo Error when running nmake.
  goto FINALLY
)

goto DONE

REM -----------------------------------
REM - Errors report
REM -----------------------------------
:ERROR

echo.
echo Some error found.
goto FINALLY

REM -----------------------------------
REM - Success report
REM -----------------------------------
:DONE

echo.
echo Done without errors.
echo Executable location: %TARGET%
goto FINALLY

REM -----------------------------------
REM - Block that is always executed
REM -----------------------------------
:FINALLY

REM Return back to original folder
cd %ORIGINAL_DIR%