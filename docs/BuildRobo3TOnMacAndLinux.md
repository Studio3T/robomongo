Building Robo 3T (macOS and Linux)  
==================

A. Prerequisites
-------------

1. Modern Compiler
A modern and complete C++17 compiler:  
```
GCC 8.0 or newer
Clang 7 (or Apple XCode 10 Clang) or newer
On Linux and macOS, the libcurl library and header is required. macOS includes libcurl.
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
   
4. Install Qt 

macOS: Qt 5.12.8   
Linux: Qt 5.9.3  

  ```sh
Example installation for macOS and Ubuntu:
    Go to http://download.qt.io/archive/qt/5.x/5.x.y/
    Download, run and install 
      qt-opensource-mac-x64-clang-5.x.y.dmg (for macOS) 
      qt-opensource-linux-x64-5.x.y.run (for Linux)
    After successful installation you should have 
      /path/to/qt-5.x.y/5.x/clang_64 (for macOS)
      /path/to/qt-5.x.y/5.x/gcc_64 (for Linux)

More information for installing on Ubuntu:
https://wiki.qt.io/Install_Qt_5_on_Ubuntu
```

5. Download and Build OpenSSL - (explained below in section B) 

B. Building Robo 3T and Dependencies
-------------
Ref: https://wiki.openssl.org/index.php/Compilation_and_Installation#OS_X  

#### 1. Build OpenSSL 

(Todo: Can we find pre-built OpenSSL? To check => https://wiki.openssl.org/index.php/Binaries)

Version 1.1.1f (Mar/2020) on macOS  
Version 1.0.2o (Mar/2018) on Linux  

**macOS:**  
```sh
wget https://www.openssl.org/source/old/1.1.1/openssl-1.1.1f.tar.gz
tar -xf openssl-1.1.1f.tar.gz
cd /opt/openssl-1.1.1f

(make clean) 
./Configure darwin64-x86_64-cc shared no-ssl2 no-ssl3 no-comp  
    // Info: With openssl version 1.1.1, configure command with rpath stopped working
    // Last working configure command with rpath was: 
    // ./Configure darwin64-x86_64-cc shared --openssldir="@rpath"

make (or sudo make)  
// Verify libssl.dylib and libcrypto.dylib file are created 
mkdir lib
cp lib*.dylib lib/

// Due to broken './Configure' command with rpath above, these extra steps are also required:
install_name_tool -id "@rpath/lib/libssl.1.1.dylib" libssl.dylib
install_name_tool -change /usr/local/lib/libcrypto.1.1.dylib @rpath/lib/libcrypto.1.1.dylib libssl.dylib
install_name_tool -id "@rpath/lib/libcrypto.1.1.dylib" libcrypto.dylib
// Finally, we should have this output using otool:
otool -L libssl.dylib 
libssl.dylib:
	@rpath/lib/libssl.1.1.dylib (compatibility version 1.1.0, current version 1.1.0)
	@rpath/lib/libcrypto.1.1.dylib (compatibility version 1.1.0, current version 1.1.0)
	/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1281.100.1)

otool -L libcrypto.dylib 
libcrypto.dylib:
	@rpath/lib/libcrypto.1.1.dylib (compatibility version 1.1.0, current version 1.1.0)
	/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1281.100.1)
```

**Linux:**

  ```sh
wget https://www.openssl.org/source/old/1.0.2/openssl-1.0.2o.tar.gz
tar -xf openssl-1.0.2o.tar.gz
cd /opt/openssl-1.0.2o
(make clean)
./config shared
make
// Verify libssl.so and libcrypto.so are created
```

#### 2. Build Robo 3T Shell (fork of MongoDB)

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

    // macOS example:
    export ROBOMONGO_CMAKE_PREFIX_PATH="/path/to/qt-5.x.y/5.x/clang_64;/opt/robo-shell;/opt/openssl-x.y.z"
    
    // Ubuntu example:
    export ROBOMONGO_CMAKE_PREFIX_PATH="/home/<user>/Qt5.x.y/5.x.y/gcc_64/;/opt/<user>/robo-shell;/opt/openssl-x.y.z"

Install pip requirements

```
// macOS / Linux
pip3 install --user -r etc/pip/compile-requirements.txt
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

#### 3. Build Robo 3T

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

