Building MongoDB 
================

Linux
-----

Clone MongoDB fork and checkout to "roboshell-v3.2" branch:

    $ git clone git@github.com:paralect/robomongo-shell.git && cd robomongo-shell
    $ git checkout roboshell-v3.2
    
Build MongoDB shell:

    $ scons mongo -j8 --release
    
#### Advanced

Build MongoDB shell in debug mode:

    $ scons mongo -j8 --dbg

Clear builds:

    $ scons -c mongo        # clears release build
    $ scons -c --dbg mongo  # clears debug build
