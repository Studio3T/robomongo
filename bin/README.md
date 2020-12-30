Bash fronted for CMake
======================

If you want to use this scripts, you need to set single environment variable:

E.g.:

    export ROBOMONGO_CMAKE_PREFIX_PATH="/path/to/qt-5/5.5/gcc_64;/path/to/robomongo-shell"

Build Robomongo:

    $ bin/configure
    $ bin/build
    
Install Robomongo:

    $ bin/install
    
Run Robomongo:

    $ bin/run    
    
Pack Robomongo:

    $ bin/pack

More: 
- [Build Robo 3T - Mac OS X and Linux](https://github.com/paralect/robomongo/blob/master/docs/BuildRobo3TOnMacAndLinux.md) 
- [Build Robo 3T - Windows](https://github.com/paralect/robomongo/blob/master/docs/BuildRobo3TOnWindows.md)
- [Unit Tests](wiki/Unit-Tests)
- [Static Code Analysis](wiki/Static-Code-Analysis)
