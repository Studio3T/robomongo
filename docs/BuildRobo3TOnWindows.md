Building Robo 3T (Windows)
==============================

A. Prerequisites
-------------

#### 0. Git

 Download Git installer from http://git-scm.com (we are using Git-2.7.0-64-bit).
  
 When you'll see "Adjusting your PATH environment" wizard page, select
 "Use Git from the Windows Command Prompt" option. This will allow to 
 execute Git from both Windows Command Prompt and Bash. This is required,
 because both MongoDB and Robo 3T uses `git` command during build process
 which must be perfomed from Windows Command Prompt (not Bash).
 
 On "Configuring the line ending conversion" wizard page, select "Checkout
 Windows-style, commit Unix-style line ending" option. This is a common mode
 for cross-platform projects and required by both MongoDB and Robo 3T.
  
 All other Git installer options are not important (use defaults).
 
 If you are using SSH clone url, start SSH agent and add your private key
 with this commands in Git Bash:
 
    $ eval `ssh-agent`       # starts ssh agent
    $ ssh-add ~/.ssh/mykey   # add your key (which can be in any folder)
  
#### 1. Visual Studio 2015 (i.e. MSVC14) Update 2 or newer

 We use Visual Studio 2015 Update 3, and recommend to use this version to save
 time for Qt compilation. Currently prebuilt binaries for Qt are available 
 from official Qt site. But you can use any newer version and compile Qt library
 yourself.

#### 2. ActivePython 2.7 
   
 Download ActivePython from http://www.activestate.com/activepython/downloads
   
 Download MSI x64 version. Use default settings in installation wizard. 
 This installer will add path to `python.exe` to your `PATH` variable.

#### 3. SCons 2.3.5 or newer

We use SCons 2.5.0.

 Download Scons from http://scons.org/tag/releases.html
   
 Use ZIP archive, not MSI
   
 Open Command Prompt, navigate to Scons directory and run:
   
    $ setup.py install
    
 This command will make `scons` command accessible from Command Prompt

#### 4. CMake 
We use CMake 3.6.0-rc3
 Download CMake from https://cmake.org/download
 
 Use installer package, it will allow you to configure your PATH variable and `cmake`
 command will be available from Command Prompt. For this set "Add CMake to the system
 PATH for current user" in the installer wizard page. 
   
#### 5. Qt 5.7

 Download Qt from http://www.qt.io/download-open-source/ (we use Qt Online installer)
   
 When installing Qt, only one component is required by Robo 3T: 
   
    Qt 5.7 
      [x] msvc2015 64-bit.
    
 This components contains prebuilt 64-bit binaries for Visual Studio 2015. 
   
 You may uncheck all other components to download as little as possible. 

#### 6. OpenSSL (1.0.1u)
Download openssl-1.0.1u (ftp://ftp.openssl.org/source/old/1.0.1/openssl-1.0.1u.tar.gz)
  
  

B. Building Robo 3T and Dependencies
-------------

#### 1. Build OpenSSL (1.0.1u)

Steps to build OpenSSL on windows:
  ```sh
Open Visual Studio tool x64 Cross Tools Command prompt
cd to the directory where you have openssl sources cd c:\myPath\openssl
perl Configure VC-WIN64A
ms\do_win64a
nmake -f ms\ntdll.mak
```

**Check Point:**
After successful build, newly created sub directory out32dll should contain dynamic lib files libeay32.lib (and libeay32.dll) and ssleay32.lib (and ssleay32.dll); and associated include files will be in newly created folder "inc32".

Helper Commands:
  ```sh
// clean to start fresh build
nmake -f ms\ntdll.mak clean
```

Refer to OpenSSL documentation for more information:  
https://wiki.openssl.org/index.php/Compilation_and_Installation#W64

#### 2. Build Robo 3T Shell (fork of MongoDB)

Clone Robo 3T Shell and checkout to roboshell-v3.4 branch:

  ```sh
  $ git clone https://github.com/paralect/robomongo-shell.git
  $ cd robomongo-shell
  $ git checkout roboshell-v3.4
  ```

Set environment variable `ROBOMONGO_CMAKE_PREFIX_PATH`, required by Robo 3T-Shell and Robo 3T build scripts, needs to be set according to the following directories:

1. Location of Qt SDK
2. Location of Robo 3T Shell
3. Location of OpenSSL

Separate directories by semicolon `;` (not colon). You can do this in Command Prompt:

    > setx ROBOMONGO_CMAKE_PREFIX_PATH "d:\Qt-5\5.7\msvc2015_64;d:\Projects\robomongo-shell;c:\myPath\openssl-1.0.1u"


Open VS2015 x64 Native Tools Command Prompt and navigate to `robomongo-shell` folder.

Build shell in release mode:

    > bin\build
    
Build shell in debug mode:

    > bin\build --dbg 
    
Note that backslash is used (`\`), and not forward slash (`/`).

Clean build files for release mode:

    > bin\clean

Clean build files for debug mode:

    > bin\clean --dbg

Refer to MongoDB documentation for additional information:
https://docs.mongodb.org/manual/contributors/tutorial/build-mongodb-from-source/#windows-specific-instructions


#### 3. Build Robo 3T   

Clone Robo 3T:

  ```sh
  $ git clone https://github.com/paralect/robomongo.git
  ```
  
Open VS2015 x64 Native Tools Command Prompt and navigate to `robomongo` folder.
 
Run configuration step:
    
    > bin\configure 
    
And finally, build Robo 3T:
    
    > bin\build 
 

**Run Robo 3T**

Install Robo 3T to `build\Release\install` folder:

    > bin\install
   
And run Robo 3T

    > \robomongo\build\Release\install\robo3t.exe

**Debug mode**

For debug mode append `debug` for each command
e.g. `bin\configure debug` or  `bin\build debug` etc..

**Helper commands**

Clean build files (in order to start build from scratch):

    > bin\clean
    

   
