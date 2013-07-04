@echo off 
setlocal enableextensions enabledelayedexpansion
rem -----------------------------------
rem - Configuration
rem -----------------------------------
rem arch_bit is a first arg
mkdir build
cd build
set arch_bit=%1
if %1. ==. (
set arch_bit=32
)
rem CMAKE_INSTALL_PREFIX
if %arch_bit% == 32 (
	cmake ../../ -G "Visual Studio 10" -DFORCE_BUILD=1
) else (
	cmake ../../ -G "Visual Studio 10 Win64" -DFORCE_BUILD=1
)
rem cmake --build . --target robomongo --config Release
cmake --build . --target install --config Release