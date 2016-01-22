rem Check that ROBOMONGO_CMAKE_PREFIX_PATH is set
if not defined ROBOMONGO_CMAKE_PREFIX_PATH (
  echo Set environment variable ROBOMONGO_CMAKE_PREFIX_PATH in order to use this script
  echo.
  echo For example, execute the following command:
  echo    setx ROBOMONGO_CMAKE_PREFIX_PATH "c:\Qt-5\5.5\msvc2013_64;c:\robomongo-shell"
  echo.
  echo You also need to reopen your Windows Command Prompt.
  exit /b 1
)

rem Build type (can be Release, Debug, RelWithDebugInfo and MinRelSize)
set BUILD_TYPE=Release

rem Build dir name (i.e. Release)
set BUILD_DIR_NAME=%BUILD_TYPE%

rem i.e. /path/to/robomongo/build
set VARIANT_DIR=%PROJECT_DIR%\build

rem i.e. /path/to/robomongo/build/debug
set BUILD_DIR=%VARIANT_DIR%\%BUILD_DIR_NAME%

rem i.e. /path/to/robomongo/build/debug/install
set INSTALL_PREFIX=%BUILD_DIR%\install

rem i.e. /path/to/robomongo/build/debug/package
set PACK_PREFIX=%BUILD_DIR%/package

rem Get value from environment variable
set PREFIX_PATH=%ROBOMONGO_CMAKE_PREFIX_PATH%

rem Create BUILD_DIR if it is not exists already
if not exist "%BUILD_DIR%" (
  md "%BUILD_DIR%"
)


