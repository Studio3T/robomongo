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
        if exist %dir_path% rm -rf %dir_path%
        mkdir %dir_path%
        cd %dir_path%
        cmake ../../ -G "Visual Studio 10" -DCPACK_GENERATOR=%cpack_generator%
        cmake --build . --target install --config Release
        cpack
    endlocal
goto:eof

