@echo off 
setlocal enableextensions enabledelayedexpansion

rem -----------------------------------
rem - Configuration
rem -----------------------------------

call clean.bat %arch_bit%
call build.bat %arch_bit%