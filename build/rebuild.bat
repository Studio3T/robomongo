@echo off 
setlocal enableextensions enabledelayedexpansion

rem -----------------------------------
rem - Configuration
rem -----------------------------------

rem arch_bit is a first arg
set arch_bit=%1

call clean.bat %arch_bit%
call build.bat %arch_bit%