@echo off
setlocal enableextensions enabledelayedexpansion

rem Path to bin and project folder
set BIN_DIR_WITH_BACKSLASH=%~dp0%
set BIN_DIR=%BIN_DIR_WITH_BACKSLASH:~0,-1%
set PROJECT_DIR=%BIN_DIR%\..

rem Run common setup code
call "%BIN_DIR%\common\setup.bat" %*
if %ERRORLEVEL% neq 0 (exit /b 1)

rem Run build with Visual Studio with RunCodeAnalysis
cd "%BUILD_DIR%"
cmake --build . --config %BUILD_TYPE% -- "/p:RunCodeAnalysis=true" "/p:CodeAnalysisRuleSet=NativeRecommendedRules.ruleset"

rem Install debug *.dll files
if "%BUILD_TYPE%" == "Debug" (
  call "%BIN_DIR%\install_debug_dlls.bat" %*
)