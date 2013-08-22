@echo off 
setlocal enableextensions enabledelayedexpansion
rem -----------------------------------
rem - Configuration
rem -----------------------------------
rem arch_bit is a first arg
mkdir build
cd build
cmake ../../ -G "Visual Studio 10"
cmake --build . --target install --config Release
cpack