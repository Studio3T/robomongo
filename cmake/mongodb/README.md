What is this? 
-------------

Files in this folder are read by FindMongoDB.cmake script.

We link with MongoDB shell using objects instead of static libs, because:

> We require the use of the 'object' mode for release builds because it is the only linking
> model that works across all of our platforms. We would like to ensure that all of our
> released artifacts are built with the same known-good-everywhere model.

This is a quote from root SConstruct file in MongoDB repository. 

It is possible to build MongoDB shell in the following way:

    $ scons mongo --link-model=static
    
Than we will have produced static libs that we can link with. But in release mode MongoDB 
SConstruct file do not support static link mode by default. This command will *not* work:

    $ scons mongo --release --link-model=static
    
We decided to use object link mode even for debug builds, in order to use single mode across 
all platforms and configurations. 

How did you receive content for this files?
-------------------------------------------

First we build MongoDB shell with the following command:

    $ scons mongo -j8 --release
    
Than we remove compiled `mongo` and run the same command again to see only final link command:

    $ rm build/opt/mongo/mongo
    $ scons mongo -j8 --release > release-link-command.txt
    
And finally we manually copy/paste list of objects to `<platform>-release.objects` file. Make sure 
that they are single-line files. 

Almost the same steps we are doing for debug builds:

    $ scons mongo -j8 --dbg
    $ rm build/debug/mongo/mongo
    $ scons mongo -j8 --dbg > debug-link-command.txt
    
And list of objects is copied to `<platform>-debug.objects` file.

We found, that on Linux (Ubuntu 14.10), list of debug objects differ from list of release objects only 
by two files. File `debugallocation.o` is linked in debug mode, but not in release. 
File `debugallocation.o` is linked in release mode, but not in debug. Both are located 
in `third_party/gperftools-2.2/src` folder. In any way, we keep separate lists of object files 
for both configurations. Still we need to test that objects are the same even across different 
versions of Linux.

