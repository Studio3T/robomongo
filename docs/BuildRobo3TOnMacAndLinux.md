Building Robo 3T (Mac OS X and Linux)  
==================

A. Prerequisites
-------------

1. Modern Compiler
A modern and complete C++17 compiler:  
```
GCC 8.0 or newer
Clang 7 (or Apple XCode 10 Clang) or newer
On Linux and macOS, the libcurl library and header is required. MacOS includes libcurl.
  Fedora/RHEL - dnf install libcurl-devel
  Ubuntu/Debian - apt-get install libcurl-dev
Python 3.7
```

For details see: https://github.com/mongodb/mongo/wiki/Build-Mongodb-From-Source  

2. Install CMake (3.10.0 or later) 

Example:  
```
sudo apt-get install build-essential
// or 
wget http://www.cmake.org/files/v3.10/cmake-3.10.0.tar.gz
tar xf cmake-3.10.0.tar.gz
cd cmake-3.10.0
./configure // macos: env CC=gcc ./bootstrap
make
sudo make install
(sudo make uninstall)
```

3. Install [Scons](http://scons.org/tag/releases.html) (3.1.2 or later) 

```
cd scons-**
python setup.py install
```
   
4. Install Qt 5.9.3

  ```sh
Example installation for MAC OSX and Ubuntu:
    Go to http://download.qt.io/archive/qt/5.x/5.x.y/
    Download, run and install 
      qt-opensource-mac-x64-clang-5.x.y.dmg (for MAC OSX) 
      qt-opensource-linux-x64-5.x.y.run (for Linux)
    After successful installation you should have 
      /path/to/qt-5.x.y/5.x/clang_64 (for MAC OSX)
      /path/to/qt-5.x.y/5.x/gcc_64 (for Linux)

More information for installing on Ubuntu:
https://wiki.qt.io/Install_Qt_5_on_Ubuntu
```

5. Download and Build OpenSSL - (explained below in section B) 

B. Building Robo 3T and Dependencies
-------------

#### Step 1. Build OpenSSL (1.0.2o)

Steps to build OpenSSL on MAC:

  ```sh
Download openssl-1.0.2o (https://www.openssl.org/source/old/1.0.2/)
tar -xvzf openssl-1.0.2o.tar.gz
cd openssl-1.0.2o
./Configure darwin64-x86_64-cc shared --openssldir="@rpath"
make (or sudo make)
mkdir lib
cp libssl*.dylib libcrypto*.dylib lib/
```
Helper Commands
  ```sh
// to start fresh build
make clean
```

Steps to build OpenSSL on Linux:

  ```sh
Download openssl-1.0.x (ftp://ftp.openssl.org/source/old/1.0.x/openssl-1.0.x.tar.gz)
tar -xvzf openssl-1.0.x.tar.gz
cd /home/<user>/Downloads/openssl-1.0.x
./config shared
make
mkdir lib
cp libssl* libcrypto* lib/
```

#### Step 2. Build Robo 3T Shell (fork of MongoDB)

Clone Robo 3T Shell and make sure you are on latest branch:

  ```sh
  $ git clone https://github.com/paralect/robomongo-shell.git
  $ cd robomongo-shell
  $ git branch   // "roboshell-v4.2"
  ```

Set special environment variable `ROBOMONGO_CMAKE_PREFIX_PATH` to point to a set of 
directories:

1. Location of Qt 5 SDK  
2. Location of Robo 3T Shell  
3. Location of OpenSSL  

Separate directories by semicolon `;` (not colon):

    // MAC OSX example:
    $ export ROBOMONGO_CMAKE_PREFIX_PATH="/path/to/qt-5.x.y/5.x/clang_64;/path/to/robomongo-shell;/path/to/openssl-1.0.xy"
    // Ubuntu example:
    $ export ROBOMONGO_CMAKE_PREFIX_PATH="/home/<user>/Qt5.x.y/5.x.y/gcc_64/;/home/<user>/robomongo-shell;/home/<user>/Downloads/openssl-1.0.xy"

Install pip requirements

```
// macOS / Linux assuming python = python 3.7
pip3 install --user -r etc/pip/dev-requirements.txt
```

```
// For ubuntu, the followings might be needed
sudo aptitude install libcurl-dev

sudo pip3 install Typing
sudo pip3 install Cheetah
```

Build Robo 3T Shell (in release mode by default):

  ```sh
  $ bin/build
  ```

Build Robo 3T Shell in debug mode:

  ```sh
  $ bin/build debug
  ```
  
Clean build files for release mode:
  ```sh
  $ bin/clean
  ```

Clean build files for debug mode:
  ```sh
  $ bin/clean debug
  ```
  
For more information refer to [Building Robo 3T Shell](BuildRobo3TShell.md) 

#### Step 3. Build Robo 3T

Download Robo 3T: 

    $ git clone https://github.com/Studio3T/robomongo
    $ cd robomongo

Run configuration step:
    
    $ bin/configure 
    
And finally, build Robo 3T:
    
    $ bin/build 

To run robomongo:

    $ bin/run
    

**Debug mode**

For debug mode append `debug` for each command
e.g. `bin/configure debug` or  `bin/build debug` etc..

**Helper commands**
    
Clean build files (in order to start build from scratch):

    $ bin/clean
    
Install Robo 3T to `build/release/install` folder:

    $ bin/install
    
Create package for your OS in `build/release/package` folder:

    $ bin/pack

**Important Notes**
- For macOS builds, it might be needed to patch QScintilla with `QScintilla-2.9.3-xcode8.patch` found at robomongo repository's root.

- For Ubuntu 16.04 builds, it has been reported that Robo 3T /bin/configure step might fail and installing `mesa-common-dev` package solves it. Details: https://github.com/paralect/robomongo/issues/1268 

- For Centos builds, Robo 3T /bin/configure step might fail due to error: `failed to find gl/gl.h`. In this case, the following packages must be installed:

```sh
$ sudo yum install mesa-libGL
$ sudo yum install mesa-libGL-devel
  ```

