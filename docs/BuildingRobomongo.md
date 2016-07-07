Building Robomongo (Mac OS X and Linux)
==================

A. Prerequisites
-------------

1. Install CMake (3.2 or later)
2. Install [Scons](http://scons.org/tag/releases.html) (2.4 or later)
3. Install Qt 5.5
4. Download and Build OpenSSL (1.0.1p) - (explained below in section B)

B. Building Robomongo and Dependencies
-------------

#### Step 1. Build OpenSSL (1.0.1p)

Steps to build OpenSSL on MAC:

  ```sh
Download openssl-1.0.1p (ftp://ftp.openssl.org/source/old/1.0.1/openssl-1.0.1p.tar.gz)
tar -xvzf openssl-1.0.1p.tar.gz
mkdir /Users/<user>/Downloads/openssl_build
cd /Users/<user>/Downloads/openssl-1.0.1p
./Configure darwin64-x86_64-cc shared --openssldir=/Users/<use>/Downloads/openssl_build
make
make install
```

Steps to build OpenSSL on Linux:

  ```sh
Download openssl-1.0.1p (ftp://ftp.openssl.org/source/old/1.0.1/openssl-1.0.1p.tar.gz)
tar -xvzf openssl-1.0.1p.tar.gz
mkdir /home/<user>/openssl_build
cd /home/<user>/Downloads/openssl-1.0.1p
./config shared --openssldir=/home/<user>/openssl_build
make install
```

#### Step 2. Build Robomongo Shell (fork of MongoDB)

Clone Robomongo Shell and checkout to `roboshell-v3.2` branch:

  ```sh
  $ git clone https://github.com/paralect/robomongo-shell.git
  $ cd robomongo-shell
  $ git checkout roboshell-v3.2
  ```

Set special environment variable `ROBOMONGO_CMAKE_PREFIX_PATH` to point to a set of 
directories:

1. Location of Qt 5 SDK  
2. Location of Robomongo Shell  
3. Location of OpenSSL  

Separate directories by semicolon `;` (not colon):

    $ export ROBOMONGO_CMAKE_PREFIX_PATH="/path/to/qt-5.5/5.5/clang_64;/path/to/robomongo-shell;/path/to/openss_build"
    

Build Robomongo Shell:

  ```sh
  $ bin/build
  ```

For more information refer to [Building Robomongo Shell](BuildingMongoDB.md) 

#### Step 3. Build Robomongo

Download Robomongo: 

    $ git clone https://github.com/paralect/robomongo
    $ cd robomongo

Run configuration step:
    
    $ bin/configure 
    
And finally, build Robomongo:
    
    $ bin/build 

To run robomongo:

    $ bin/run
    

**Helper commands**
    
Clean build files (in order to start build from scratch):

    $ bin/clean
    
Install Robomongo to `build/release/install` folder:

    $ bin/install
    
Create package for your OS in `build/release/package` folder:

    $ bin/pack
    
