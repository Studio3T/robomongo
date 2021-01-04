@echo off
setlocal enableextensions enabledelayedexpansion

set CPP_VER=c++17

set FILE_OR_DIR=..\src\robomongo\
set PARAM_1=%1
if not "%PARAM_1%" == "" (
  set FILE_OR_DIR=%PARAM_1%
)

rem Run analysis
echo.
echo ---------------------- Running cppcheck ----------------------
echo.
call cppcheck %FILE_OR_DIR% --enable=all --std=%CPP_VER%
echo.
echo ---------------------- End of cppcheck ----------------------
echo.
echo ---------------------- Info ----------------------
call cppcheck --version
echo Command called: cppcheck %FILE_OR_DIR% --enable=all --std=%CPP_VER%