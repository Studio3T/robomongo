@echo off 
setlocal enableextensions enabledelayedexpansion

rem -----------------------------------
rem - Configuration
rem -----------------------------------
rem arch_bit is a first arg

call :createPackage build_nsis NSIS
call :createPackage build_zip ZIP
goto :eof

:createPackage
    setlocal
        set dir_path=%1
        set cpack_generator=%2
        if exist %dir_path% rmdir %dir_path% /s /q
        mkdir %dir_path%
        cd %dir_path%
        
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
        cd ../
    endlocal
goto:eof

