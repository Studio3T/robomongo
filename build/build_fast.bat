@echo off 
setlocal enableextensions enabledelayedexpansion

rem -----------------------------------
rem - Configuration
rem -----------------------------------
rem arch_bit is a first arg

set dir_path=build_releases
if exist %dir_path% rmdir %dir_path% /s /q
mkdir %dir_path%
cd %dir_path%

call :createPackage NSIS
call :createPackage ZIP
goto :eof

:createPackage
    setlocal     
        set cpack_generator=%1
        if exist "%programfiles%\Microsoft Visual Studio 11.0" (
            cmake ../../ -G "Visual Studio 11" -DCPACK_GENERATOR=%cpack_generator%
        ) else ( 
            if exist "%programfiles%\Microsoft Visual Studio 10.0" (
                cmake ../../ -G "Visual Studio 10" -DCPACK_GENERATOR=%cpack_generator%
            ) else ( 
                echo Error Visual Studio not founded.
                cd ../
                if exist %dir_path% rmdir %dir_path% /s /q                
            goto:eof
            )
        )
        
        cmake --build . --target install --config Release
        cpack
    endlocal
goto:eof

