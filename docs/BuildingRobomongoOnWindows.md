Building Robomongo for Windows
==============================

Prerequisites
-------------

#### 0. Git

 Download Git installer from http://git-scm.com (we are using Git-2.7.0-64-bit).
  
 When you'll see "Adjusting your PATH environment" wizard page, select
 "Use Git from the Windows Command Prompt" option. This will allow to 
 execute Git from both Windows Command Prompt and Bash. This is required,
 because both MongoDB and Robomongo uses `git` command during build process
 which must be perfomed from Windows Command Prompt (not Bash).
 
 On "Configuring the line ending conversion" wizard page, select "Checkout
 Windows-style, commit Unix-style line ending" option. This is a common mode
 for cross-platform projects and required by both MongoDB and Robomongo.
  
 All other Git installer options are not important (use defaults).
  
#### 1. Visual Studio 2013 (i.e. MSVC12) Update 4 or newer

 We use Visual Studio 2013 Update 5, and recommend to use this version to save
 time for Qt compilation. Currently prebuilt binaries for Qt are available 
 from official Qt site. But you can use any newer version and compile Qt library
 yourself.
 
 If you already have VS2013, you can download Update 5 here: 
 https://www.microsoft.com/en-us/download/details.aspx?id=48129
 
 Update 4 or newer version is required to compile MongoDB.

#### 2. ActivePython 2.7 
   
 Download ActivePython from http://www.activestate.com/activepython/downloads
   
 Download MSI x64 version. Use default settings in installation wizard. 
 This installer will add path to `python.exe` to your `PATH` variable.

#### 3. Scons 2.3.4 

 Download Scons from http://scons.org/tag/releases.html
   
 Use ZIP archive, not MSI (we use scons-2.3.4.zip)
   
 Open Command Prompt, navigate to Scons directory and run:
   
    $ setup.py install
    
 This command will make `scons` command accessible from Command Prompt

#### 4. CMake 
 Download CMake from https://cmake.org/download (we use cmake-3.3.2-win32-x86.exe)
 
 Use installer package, it will allow you to configure your PATH variable and `cmake`
 command will be available from Command Prompt. For this set "Add CMake to the system
 PATH for current user" in the installer wizard page. 
   
#### 5. Qt 5.5

 Download Qt from http://www.qt.io/download-open-source/ (we use Qt Online installer)
   
 When installing Qt, only one component is required by Robomongo: 
   
    Qt 5.5 
      [x] msvc2013 64-bit.
    
 This components contains prebuilt 64-bit binaries for Visual Studio 2013. 
   
 You may uncheck all other components to download as little as possible. 



## Build Robomongo Shell (fork of MongoDB)

Refer to MongoDB documentation for additional information:
https://docs.mongodb.org/manual/contributors/tutorial/build-mongodb-from-source/#windows-specific-instructions

Open VS2013 x64 Native Tools Command Prompt and navigate to `robomongo-shell` folder.

Build shell:

    $ scons mongo --release -j8


   
   