Robomongo
=========

Robomongo &mdash; is a shell-centric crossplatform MongoDB management tool. <br />
Visit our site: [www.robomongo.org](http://www.robomongo.org)

Download
========

You can download compiled version at this page:<br />
https://www.dropbox.com/sh/u0s0i8e4m0a8i9f/oxtqKHPUZ8

Only Windows binaries available for now. 


Building
========

You are lucky enough, if prebuild libraries (that are in `libs` folder) are 
already available and match your OS/Compiler. For most of you it's not &mdash; 
you need to build Robomongo dependencies, before building Robomongo itself.

Here is a detailed instructions on building Robomongo dependencies for Linux and/or Mac OS X:<br />
[Building Robomongo and Dependencies (for Linux and Mac OS X)]
(https://github.com/paralect/robomongo/wiki/Building-Robomongo-and-Dependencies-(for-Linux-and-Mac-OS-X\))



Windows
-------

The following steps assume that all dependencies already compiled.

Prerequisites:

* Qt should be compiled with VC2010. Tested with Qt 4.8
* Your PATH variable should have Qt bin folder
* Visual Studio 2010 should be installed and VC should be in this location: %ProgramFiles%\Microsoft Visual Studio 10.0\VC. Otherwise you need to modify VISUALC_PATH in build script.

Compiling:

    > cd build
    > build.bat

Executable will be placed to: target/debug/app/out



Linux and OS X
---------------

The following steps assume that all dependencies already compiled.

Prerequisites:

* Qt should be installed. Tested with Qt 4.8
* Your PATH variable should have Qt bin folder

Compiling:

    $ cd build
    $ chmod u+x build.sh
    $ ./build.sh

Executable will be placed to: target/debug/app/out
