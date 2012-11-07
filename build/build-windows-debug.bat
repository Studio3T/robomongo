@echo off 
setlocal enableextensions enabledelayedexpansion

REM -----------------------------------
REM - Configuration
REM -----------------------------------

REM Visual C++ directory
set VISUALC_DIR=%ProgramFiles%\Microsoft Visual Studio 10.0\VC

REM Store current directory in order to return to it after
set ORIGINAL_DIR=%CD%

REM Find project root folder
set PROJECT_ROOT=%CD%\..\

echo Visual C folder: %VISUALC_DIR%

REM -----------------------------------
REM - Preparation
REM -----------------------------------

REM Create target folder
set OUTPUT=%PROJECT_ROOT%target\debug
if not exist %OUTPUT% mkdir %OUTPUT%
if %ERRORLEVEL% neq 0 goto ERROR

echo Output folder: %OUTPUT%

REM Go to /target/debug folder
cd %PROJECT_ROOT%target\debug
if %ERRORLEVEL% neq 0 goto ERROR

echo.

REM -----------------------------------
REM - QMake
REM -----------------------------------

REM Run qmake
qmake %PROJECT_ROOT%src\play.pro -r -spec win32-msvc2010 "CONFIG+=debug" "CONFIG+=declarative_debug"
if %ERRORLEVEL% neq 0 (
  echo.
  echo Error when running qmake.
  goto FINALLY
)

REM -----------------------------------
REM - Visual C nmake
REM -----------------------------------

IF "!%VSINSTALLDIR%!" == "!!" (

  echo Settings Visual C environment variables...
  call "%VISUALC_DIR%\bin\vcvars32.bat"
  if %ERRORLEVEL% neq 0 goto ERROR
  
) else ( 
  echo Skipping Visual C environment setup ^(already configured^)
)

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
echo Executable location: %OUTPUT%
goto FINALLY

REM -----------------------------------
REM - Block that is always executed
REM -----------------------------------
:FINALLY

REM Return back to original folder
cd %ORIGINAL_DIR%