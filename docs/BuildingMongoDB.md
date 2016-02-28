Building MongoDB 
================

Linux and Mac
-------------

Clone MongoDB fork and checkout to "roboshell-v3.2" branch:

    $ git clone https://github.com/paralect/robomongo-shell.git
    $ cd robomongo-shell
    $ git checkout roboshell-v3.2
    
Build MongoDB shell in release mode:

    $ bin/build

Clean build files for release mode:

    $ bin/clean
    
#### Advanced

Here is command that is executed by `bin/build` script:

    $ scons mongo -j8 --release --osx-version-min=10.9
    
Argument `--osx-version-min` is required only for Mac OS.
    
Build MongoDB shell in debug mode:

    $ scons mongo -j8 --dbg

Clear builds:

    $ scons -c mongo        # clears release build
    $ scons -c --dbg mongo  # clears debug build
