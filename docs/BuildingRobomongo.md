Building Robomongo
==================

Download and build Robomongo shell (our fork of MongoDB):

    $ git clone https://github.com/paralect/robomongo-shell
    $ cd robomongo-shell
    $ git checkout roboshell-v3.2
    $ scons mongo --release -j8


Download and build Robomongo:

    $ git clone https://github.com/paralect/robomongo
    $ cd robomongo
    $ git checkout v0.9
    $ export ROBOMONGO_CMAKE_PREFIX_PATH="/path/to/qt-5.5/5.5/clang_64;/path/to/robomongo-shell"
    $ bin/configure 
    $ bin/build 
    
    
