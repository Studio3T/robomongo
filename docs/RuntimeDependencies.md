Runtime Dependencies
====================

This information is for maintainers. If you want to just use Robomongo - download 
prebuild packages for your platform.

Runtime dependencies that are not privided by platforms but required by Robomongo are:

1. Qt libraries
2. Qt plugins

Diagnostic of dependencies
--------------------------

#### Linux

a. View dependencies of executable or shared library: 

    $ ldd robomongo
    $ readelf -d robomongo

b. View dependency tree of executable or shared library:  

    $ lddtree robomongo
    $ ldd -v robomongo

c. View RPATH or RUNPATH records of executable or shared library:

    $ objdump -x bin/robomongo | grep "RPATH\|RUNPATH"
    $ readelf -d bin/robomongo | grep RPATH     
    
For Robomongo binary it should be `$ORIGIN/../lib`

d. Enable extensive logging for run-time shared library search 

    $ export LD_DEBUG=files
    $ bin/robomongo
    
Source: 
http://tldp.org/HOWTO/Program-Library-HOWTO/shared-libraries.html

e. Modify RPATH using `chrpath` and the dynamic linker and RPATH using paths `patchelf`
    
    $ patchelf --set-rpath '$ORIGIN/../lib' robo
    
More: https://nixos.org/patchelf.html

    $ chrpath -r "/somepath/lib/" robomongo

More:
https://linux.die.net/man/1/chrpath
http://www.programering.com/a/MTOwcDNwATU.html	// examples

#### Mac OS X

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
    

