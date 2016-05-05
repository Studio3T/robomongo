Building Robomongo
==================

#### Prerequisites

1. CMake (3.2 or later)
2. Qt (5.5 or later)

#### Step 1.

Download and build Robomongo Shell (our fork of MongoDB). 
Refer to [Building Robomongo Shell](BuildingMongoDB.md) document.

#### Step 2.

Download Robomongo: 

    $ git clone https://github.com/paralect/robomongo
    $ cd robomongo

Set special environment variable `ROBOMONGO_CMAKE_PREFIX_PATH` to point to a set of 
directories:

1. Location of Qt 5 SDK
2. Location of Robomongo Shell **source root** folder (built in "Step 1")

Separate directories by semicolon `;` (not colon):

    $ export ROBOMONGO_CMAKE_PREFIX_PATH="/path/to/qt-5.5/5.5/clang_64;/path/to/robomongo-shell"
    
Run configuration step:
    
    $ bin/configure 
    
And finally, build Robomongo:
    
    $ bin/build 
    
<br>    
## Helper commands

Execute just built Robomongo:

    $ bin/run
    
Clean build files (in order to start build from scratch):

    $ bin/clean
    
Install Robomongo to `build/release/install` folder:

    $ bin/install
    
Create package for your OS in `build/release/package` folder:

    $ bin/pack
    
