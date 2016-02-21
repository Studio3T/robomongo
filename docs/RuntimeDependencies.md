Runtime Dependencies
====================

This information is for maintainers. If you want to just use Robomongo - download 
prebuild packages for your platform.

Runtime dependencies that are not privided by platforms but required by Robomongo are:

1. Qt libraries
2. Qt plugins

Diagnostic of dependencies
--------------------------

##### Linux

View dependencies of executable or shared library:

    $ ldd robomongo
    
View RPATH or RUNPATH records of executable or shared library:

    $ objdump -x bin/robomongo | grep "RPATH\|RUNPATH"
    
For Robomongo binary it should be `$ORIGIN/../lib`

##### Mac OS X

Show dependencies of executable or shared library:

    $ otool -L robomongo

View LC_RPATH records of executable or shared library:

    $ otool -l robomongo | grep -C2 RPATH
    
(option `-C2` asks grep to output 2 context lines before and after the match)

For Robomongo binary it should be `$executable_path/../Frameworks`


Diagnostic of Qt plugins
------------------------

Run Robomongo in the following way to see how plugins are resolved:

    $ QT_DEBUG_PLUGINS=1 ./robomongo
    

