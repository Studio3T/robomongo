@echo off
setlocal enableextensions enabledelayedexpansion

set CPP_VER=c++17
set DIR=..\src\robomongo\

rem Run analysis
echo.
echo ---------------------- Running cppcheck ----------------------
echo.
call cppcheck %DIR% --enable=all --std=%CPP_VER%
echo.
echo ---------------------- End of cppcheck ----------------------
echo.
echo ---------------------- Info ----------------------
call cppcheck --version
echo Command called: cppcheck %DIR% --enable=all --std=%CPP_VER%
