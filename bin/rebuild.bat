@echo off
setlocal enableextensions enabledelayedexpansion

call "clean.bat" %*
call "configure.bat" %*
call "build.bat" %*