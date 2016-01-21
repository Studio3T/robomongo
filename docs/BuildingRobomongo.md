Building Robomongo
==================

#### Step 1.

Download and build Robomongo shell (our fork of MongoDB). 
Refer to [Building MongoDB fork](BuildingMongoDB.md) document in this folder.

#### Step 2.

Download Robomongo and checkout to `v0.9` branch:

    $ git clone https://github.com/paralect/robomongo
    $ cd robomongo
    $ git checkout v0.9

Set special environment variable `ROBOMONGO_CMAKE_PREFIX_PATH` to point to set of 
directories:

1. Location of Qt SDK
2. Location of Robomongo Shell (built in "Step 1")

Separate directories by semicolon `;` (not colon):

    $ export ROBOMONGO_CMAKE_PREFIX_PATH="/path/to/qt-5.5/5.5/clang_64;/path/to/robomongo-shell"
    
Run configuration step:
    
    $ bin/configure 
    
And finally, build Robomongo:
    
    $ bin/build 
    
## Helper commands

Execute just built Robomongo:

    $ bin/run
    
Clean build files (in order to start build from scratch):

    $ bin/clean
    
Install Robomongo to `build/release/install` folder:

    $ bin/install
    
Create package for your OS in `build/release/package` folder:

    $ bin/pack
    