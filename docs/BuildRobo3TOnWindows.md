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
  
#### 1. Visual Studio 2017 version 15.9 or newer (complete C++17 compiler)

 We use Visual Studio 2017 version 15.9.22, and recommend to use this version to save
 time for Qt compilation. Currently prebuilt binaries for Qt are available 
 from official Qt site. But you can use any newer version and compile Qt library
 yourself.

#### 2. ActivePython 3.7.4
   
 Download ActivePython from http://www.activestate.com/activepython/downloads
   
 Download MSI x64 version. Use default settings in installation wizard. 
 This installer will add path to `python.exe` to your `PATH` variable.

#### 3. SCons 3.1.2 or newer

We use SCons 3.1.2

 Download Scons from http://scons.org/tag/releases.html
   
 Use ZIP archive, not MSI
   
 Open Command Prompt, navigate to Scons directory and run:
   
    $ setup.py install
    
 This command will make `scons` command accessible from Command Prompt

#### 4. CMake 
We use CMake 3.10.0
 Download CMake from https://cmake.org/download
 
 Use installer package, it will allow you to configure your PATH variable and `cmake`
 command will be available from Command Prompt. For this set "Add CMake to the system
 PATH for current user" in the installer wizard page. 
   
#### 5. Qt 5.9.3

 Download and install Qt from http://download.qt.io/archive/qt 
 Run the installer and select only 'msvc2015 64-bit'

#### 6. OpenSSL (1.1.1f)
Download openssl from https://www.openssl.org/source/old/1.1.1/openssl-1.1.1f.tar.gz  

B. Building Robo 3T and Dependencies
-------------  

#### 1. Build OpenSSL

Steps to build OpenSSL on windows:  
- Ensure you have perl installed on your machine (e.g. ActiveState or Strawberry), and available on your %PATH%  
- Ensure you have NASM installed on your machine, and available on your %PATH%  

```sh
Open Visual Studio tool x64 Cross Tools Command prompt
cd to the directory where you have openssl sources cd c:\myPath\openssl
perl Configure VC-WIN64A
(nmake clean)
nmake
// Verify these files are created: 
libcrypto-1_1x64.dll, libssl-1_1-x64.dll (with all the build additionals such as .pdb .lik or static .lib)
```  

Refer to OpenSSL documentation for more information:  
https://wiki.openssl.org/index.php/Compilation_and_Installation#OpenSSL_1.1.0  

#### 2. Build Robo 3T Shell (fork of MongoDB)

Clone Robo 3T Shell and make sure to be at `roboshell-v4.2` branch:

  ```sh
  $ git clone https://github.com/paralect/robomongo-shell.git
  $ cd robomongo-shell
  $ git branch  // roboshell-v4.2
  ```

Set environment variable `ROBOMONGO_CMAKE_PREFIX_PATH`, required by Robo 3T-Shell and Robo 3T build scripts, needs to be set according to the following directories:

1. Location of Qt SDK
2. Location of Robo 3T Shell
3. Location of OpenSSL

**Important:**   
Make sure to move all your repos (robo, openssl, robo-shell etc..) into root directory of your drives (e.g. e:\robo-shell) and keep short repo names (e.g. robo-shell instead of robomongo-shell) otherwise you might end up with this error `If you see fatal error LNK1170: line in command file contains 131071 or more characters`

Separate directories by semicolon `;` (not colon). You can do this in Command Prompt:

    > setx ROBOMONGO_CMAKE_PREFIX_PATH "e:\Qt\Qt5.9.3\5.9.3\msvc2015_64;e:\robo-shell;e:\openssl-x.y.z"  

Open '**a new**' VS2017 x64 Native Tools Command Prompt and navigate to `robomongo-shell` folder.


**Note:**  

Run 
`pip3 install --user -r ../etc/pip/dev-requirements.txt`  

If you have this type of errors:  

```
// 1st error
ImportError: No module named typing
// Solution: 
pip install Typing
pip install Cheetah

// 2nd error: shell: cheetah module not found
// Solution: 
copy C:\Users\<user>\AppData\Roaming\Python\Python27\site-packages
to C:\Python27\Lib\site-packages 

```

Build shell in release mode:

    > bin\build
    
Build shell in debug mode:

    > bin\build debug 
    
Note that backslash is used (`\`), and not forward slash (`/`).

Clean build files for release mode:

    > bin\clean

Clean build files for debug mode:

    > bin\clean debug

Refer to MongoDB documentation for additional information:
https://docs.mongodb.org/manual/contributors/tutorial/build-mongodb-from-source/#windows-specific-instructions


#### 3. Build Robo 3T   

Clone Robo 3T:

  ```sh
  $ git clone https://github.com/paralect/robomongo.git
  ```
  
Open VS2017 x64 Native Tools Command Prompt and navigate to `robomongo` folder.
 
Run configuration step:
    
    > bin\configure 
    
And finally, build Robo 3T:
    
    > bin\build 

```
// If you see fatal error LNK1170: line in command file contains 131071 or more characters
Solution-1: 
1. Move all your repos (robo, openssl, robo-shell etc..) into root directory of your drives.
2. Use short names e.g. robo-shell instead of robomongo-shell.
3. Update ROBOMONGO_CMAKE_PREFIX_PATH env. variable and try a fresh (clean) build.

Solution-2: 
1. Create environment variable MongoDB_OBJECTS, set a short path e.g. "C:/"
2. Open a new VS2017 x64 Native Tools Command Prompt and build Robo 3T Shell 
3. Robo 3T Shell build script will copy the object files into MongoDB_OBJECTS directory
4. Build Robo 3T again (clean build might be needed)
```

**Run Robo 3T**

Install Robo 3T to `build\Release\install` folder:

    > bin\install
   
And run Robo 3T

    > \robomongo\build\Release\install\robo3t.exe

**Package Robo 3T**

Install Robo 3T to `build\Release\install` folder:

    > bin\pack

Note: nsis-2.46-setup.exe with Full setup must be present.


**Debug mode**

For debug mode append `debug` for each command
e.g. `bin\configure debug` or  `bin\build debug` etc..

**Helper commands**

Clean build files (in order to start build from scratch):

    > bin\clean
    

   
