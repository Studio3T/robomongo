Building Robomongo for Windows
==============================

### Build Robomongo Shell (fork of MongoDB)

Refer to MongoDB documentation:
https://docs.mongodb.org/manual/contributors/tutorial/build-mongodb-from-source/#windows-specific-instructions

1. Visual Studio 2013 (i.e. MSVC12) Update 4 or later.

2. ActivePython 2.7 (http://www.activestate.com/activepython/downloads). 
   Download MSI x64 version. Use default settings in installation wizard. 
   This installer will add path to `python.exe` to your `PATH` variable.
   
3. Scons 2.3.4 (download zip archive, not MSI)
   (We were unable to build MongoDB with Scons 2.3.1, so please use 2.3.4)
   Open command line, navigate to scons directory and run:
   
```   
   $ setup.py install
```

Open VS2013 x64 Native Tools Command Prompt and navigate to `robomongo-shell` folder.

Build shell:

   $ scons mongo --release -j8

   
   