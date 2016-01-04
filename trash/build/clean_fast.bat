@echo off 
setlocal enableextensions enabledelayedexpansion

rem -----------------------------------
rem - Configuration
rem -----------------------------------
rem remove target folder

call :deleteDir build_releases
goto :eof

:deleteDir
    setlocal
        set dir_path=%1
        if exist %dir_path% rmdir %dir_path% /s /q
        if %ERRORLEVEL% neq 0 (
          echo.
          echo Error when removing !TARGET!.
          exit /b 1
          pause
        )
    endlocal
goto:eof
