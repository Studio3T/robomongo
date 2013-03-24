Robomongo
=========

Robomongo is a shell-centric crossplatform MongoDB management tool. 

Download
========

You can download compiled version at this page:

https://www.dropbox.com/sh/u0s0i8e4m0a8i9f/oxtqKHPUZ8

Only Windows binaries available for now. 


Building
========

Windows
-------

Prerequisites:

* Qt should be compiled with VC2010. Tested with Qt 4.8
* Your PATH variable should have Qt bin folder
* Visual Studio 2010 should be installed and VC should be in this location: %ProgramFiles%\Microsoft Visual Studio 10.0\VC. Otherwise you need to modify VISUALC_PATH in build script.

Compiling:

    > cd build
    > build.bat

Executable will be placed to: target/debug/app/out



Linux and OS X
-------

Prerequisites:

* Qt should be installed. Tested with Qt 4.8
* Your PATH variable should have Qt bin folder

Compiling:

    $ cd build
    $ chmod u+x build.sh
    $ ./build.sh

Executable will be placed to: target/debug/app/out

Build Scripts
-------------

Build scripts located in ./build folder. For Linux/OSX use `*.sh` scripts, for Windows use `*.bat` scripts.

### Build

Build with debug symbols:

    $ ./build.sh -d
    
Build in release mode:
 
    $ ./build.sh -r
    
Build both in debug and release modes:

    $ ./build.sh -a
    
### Rebuild    
    
Rebuild script first cleans output, than builds. This is a shortcat for:

    $ ./clean.sh <ARGS>
    $ ./build.sh <ARGS>
    
Rebuild with debug symbols:
   
    $ ./rebuild.sh -d
    
Rebuild in release mode:
   
    $ ./rebuild.sh -r
    
Rebuild both in debug and release modes:

    $ ./rebuild.sh -a
    
### Clean    
    
Clean debug output:
    
    $ ./clean.sh -d
    
Clean release output:

    $ ./clean.sh -r
    
Clean all output (both debug and release)

    $ ./clean.sh -a
