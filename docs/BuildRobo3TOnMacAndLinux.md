Building Robo 3T (Mac OS X and Linux)  
==================

A. Prerequisites
-------------

1. Modern Compiler   
A modern and complete C++11 compiler. One of the following is required:  
```
GCC 5.3.0  
Clang 3.4 (or Apple XCode 5.1.1 Clang) or newer  
```

2. Install CMake (3.2 or later)
3. Install [Scons](http://scons.org/tag/releases.html) (2.4 or later)
4. Install Qt 5.7

  ```sh
Example installation for MAC OSX and Ubuntu:
    Go to http://download.qt.io/archive/qt/5.7/5.7.0/
    Download, run and install 
      qt-opensource-mac-x64-clang-5.7.0.dmg (for MAC OSX) 
      qt-opensource-linux-x64-5.7.0.run (for Linux)
    After successful installation you should have 
      /path/to/qt-5.7.0/5.7/clang_64 (for MAC OSX)
      /path/to/qt-5.7.0/5.7/gcc_64 (for Linux)

More information for installing on Ubuntu:
https://wiki.qt.io/Install_Qt_5_on_Ubuntu
```

5. Download and Build OpenSSL (1.0.1u) - (explained below in section B)

B. Building Robo 3T and Dependencies
-------------

#### Step 1. Build OpenSSL (1.0.1u)

Steps to build OpenSSL on MAC:

  ```sh
Download openssl-1.0.1u (ftp://ftp.openssl.org/source/old/1.0.1/openssl-1.0.1u.tar.gz)
tar -xvzf openssl-1.0.1u.tar.gz
cd /Users/<user>/Downloads/openssl-1.0.1u
./Configure darwin64-x86_64-cc shared --openssldir="@rpath"
make (or sudo make)
mkdir lib
cp libssl.1.0.0.dylib libcrypto.1.0.0.dylib lib/
```
Helper Commands
  ```sh
// to start fresh build
make clean
```


Steps to build OpenSSL on Linux:

  ```sh
Download openssl-1.0.1u (ftp://ftp.openssl.org/source/old/1.0.1/openssl-1.0.1u.tar.gz)
tar -xvzf openssl-1.0.1u.tar.gz
cd /home/<user>/Downloads/openssl-1.0.1u
./config shared
make
mkdir lib
cp libssl* libcrypto* lib/
```

#### Step 2. Build Robo 3T Shell (fork of MongoDB)

Clone Robo 3T Shell and checkout to `roboshell-v3.4` branch:

  ```sh
  $ git clone https://github.com/paralect/robomongo-shell.git
  $ cd robomongo-shell
  $ git checkout roboshell-v3.4
  ```

Set special environment variable `ROBOMONGO_CMAKE_PREFIX_PATH` to point to a set of 
directories:

1. Location of Qt 5 SDK  
2. Location of Robo 3T Shell  
3. Location of OpenSSL  

Separate directories by semicolon `;` (not colon):

    // MAC OSX example:
    $ export ROBOMONGO_CMAKE_PREFIX_PATH="/path/to/qt-5.7.0/5.7/clang_64;/path/to/robomongo-shell;/path/to/openssl-1.0.1u"
    // Ubuntu example:
    $ export ROBOMONGO_CMAKE_PREFIX_PATH="/path/to/qt-5.7.0/5.7/gcc_64;/path/to/robomongo-shell;/path/to/openssl-1.0.1u"


Build Robo 3T Shell:

  ```sh
  $ bin/build
  ```

For more information refer to [Building Robo 3T Shell](BuildingMongoDB.md) 

#### Step 3. Build Robo 3T

Download Robo 3T: 

    $ git clone https://github.com/paralect/robomongo
    $ cd robomongo

Run configuration step:
    
    $ bin/configure 
    
And finally, build Robo 3T:
    
    $ bin/build 

To run robomongo:

    $ bin/run
    

**Helper commands**
    
Clean build files (in order to start build from scratch):

    $ bin/clean
    
Install Robo 3T to `build/release/install` folder:

    $ bin/install
    
Create package for your OS in `build/release/package` folder:

    $ bin/pack

**Important Notes**
- For Ubuntu 16.04 builds, it has been reported that Robo 3T /bin/configure step might fail and installing mesa-common-dev package solves it. Details: https://github.com/paralect/robomongo/issues/1268 

- For Centos builds, Robo 3T /bin/configure step might fail due to error: `failed to find gl/gl.h`. In this case, the following packages must be installed:

```sh
$ sudo yum install mesa-libGL
$ sudo yum install mesa-libGL-devel
  ```

