@echo off
setlocal enableextensions enabledelayedexpansion

rem Path to bin and project folder
set BIN_DIR_WITH_BACKSLASH=%~dp0%
set BIN_DIR=%BIN_DIR_WITH_BACKSLASH:~0,-1%
set PROJECT_DIR=%BIN_DIR%\..

rem Run common setup code
call "%BIN_DIR%\common\setup.bat" %*
if %ERRORLEVEL% neq 0 (exit /b 1)

rem Run Visual Studio with RunCodeAnalysis
cd "%BUILD_DIR%"
set RULE_SET="/p:CodeAnalysisRuleSet=NativeRecommendedRules.ruleset"
set OUT_FILE=vs-code-analysis-%BUILD_TYPE%.txt

echo ------------- Running Visual Studio with RunCodeAnalysis -------------
echo Result will be written into file "%OUT_FILE%" in the end.
echo ...
cmake --build . --config %BUILD_TYPE% -- "/p:RunCodeAnalysis=true" %RULE_SET% > %BIN_DIR%\%OUT_FILE%
  rem Rules: C:\Program Files (x86)\Microsoft Visual Studio 14.0\Team Tools\Static Analysis Tools\Rule Sets
  rem        https://docs.microsoft.com/en-us/visualstudio/code-quality/rule-set-reference?view=vs-2019

echo ------------- End of RunCodeAnalysis -------------
echo.
echo ------------- Info -------------
echo Mode    : %BUILD_TYPE%
echo RULE_SET: %RULE_SET% (see the script comments for more info)