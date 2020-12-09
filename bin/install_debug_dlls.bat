@echo off
setlocal enableextensions enabledelayedexpansion

rem // This script installs debug *.dll files
rem -------------------------------------------

set INSTALL_DIR=%BUILD_DIR%\src\robomongo\Debug\
if not exist %INSTALL_DIR%*.dll (
  echo ----------------------------------------
  echo Installing debug *.dll files ...  
  echo INSTALL_DIR: %INSTALL_DIR%
  echo ----------------------------------------  
)

rem // Find OpenSSL and Qt paths
for %%i in (%ROBOMONGO_CMAKE_PREFIX_PATH%) do (
  set xx=%%i
  if not "!xx!"=="!xx:ssl=!" ( set OPENSSL_DIR=!xx! )
  if not "!xx!"=="!xx:qt=!" ( set Qt_DIR=!xx! )
)
set OPENSSL_DIR=%OPENSSL_DIR: =%
set Qt_DIR=%Qt_DIR: =%

rem // Copy dll files
for /d %%a in (
  "%VS140COMNTOOLS%\..\..\VC\redist\debug_nonredist\x64\Microsoft.VC140.DebugCRT\msvcp140d.dll"
  "%VS140COMNTOOLS%\..\..\VC\redist\debug_nonredist\x64\Microsoft.VC140.DebugCRT\vcruntime140d.dll"
  "%OPENSSL_DIR%\libssl-1_1-x64.dll"
  "%OPENSSL_DIR%\libcrypto-1_1-x64.dll"
  "%Qt_DIR%\bin\Qt5Cored.dll"
  "%Qt_DIR%\bin\Qt5Guid.dll"
  "%Qt_DIR%\bin\Qt5Networkd.dll"
  "%Qt_DIR%\bin\Qt5Widgetsd.dll"
  "%Qt_DIR%\bin\Qt5Positioningd.dll"
  "%Qt_DIR%\bin\Qt5Qmld.dll"
  "%Qt_DIR%\bin\Qt5Quickd.dll"
  "%Qt_DIR%\bin\Qt5QuickWidgetsd.dll"
  "%Qt_DIR%\bin\Qt5WebChanneld.dll"
  "%Qt_DIR%\bin\Qt5WebEngineCored.dll"
  "%Qt_DIR%\bin\Qt5WebEngineWidgetsd.dll"
  "%Qt_DIR%\bin\Qt5PrintSupportd.dll"  
) do (
  if not exist !INSTALL_DIR!%%~NXa (
    xcopy %%a !INSTALL_DIR! /d /y
  )
)

if not exist %INSTALL_DIR%\imageformats\qgifd.dll (
  xcopy /s "%Qt_DIR%\plugins\imageformats" %INSTALL_DIR%\imageformats\ /d /y
)

if not exist %INSTALL_DIR%\platforms\qminimald.dll (
  xcopy /s "%Qt_DIR%\plugins\platforms" %INSTALL_DIR%\platforms\ /d /y
)