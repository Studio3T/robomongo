Runtime Dependencies
====================

This information is for maintainers. If you want to just use Robomongo - download 
prebuild packages for your platform.

Runtime dependencies that are not privided by platforms but required by Robomongo are:

1. Qt libraries
2. Qt plugins

Diagnostic of dependencies
--------------------------

### Windows 

GUI: http://dependencywalker.com/

CMD LINE:   
Open Visual Studio Native command prompt  
`dumpbin /dependents robo3t.exe`

### Linux

#### a. View dependencies of executable or shared library: 

    $ ldd robomongo
    $ readelf -d robomongo

#### b. View dependency tree of executable or shared library:  

    $ lddtree robomongo
    $ ldd -v robomongo

#### c. View RPATH or RUNPATH records of executable or shared library:

    $ objdump -x bin/robomongo | grep "RPATH\|RUNPATH"
    $ readelf -d bin/robomongo | grep RPATH     
    
For Robomongo binary it should be `$ORIGIN/../lib`

#### d. Enable extensive logging for run-time shared library search 

    $ export LD_DEBUG=files
    $ bin/robomongo
    
Source: 
http://tldp.org/HOWTO/Program-Library-HOWTO/shared-libraries.html

#### e. Modify RPATH using `chrpath` and the dynamic linker and RPATH using paths `patchelf`
    
    $ patchelf --set-rpath '$ORIGIN/../lib' robo
    
More: https://nixos.org/patchelf.html

    $ chrpath -r "/somepath/lib/" robomongo

More:
https://linux.die.net/man/1/chrpath
http://www.programering.com/a/MTOwcDNwATU.html	// examples

#### f. Show symbols of shared lib file

Error: 
```
/opt/robo-shell/src/mongo/util/net/ssl_manager_openssl.cpp:667: 
undefined reference to `SSL_library_init'
```

How to debug:
```
openssl-1.1.1f: nm -gD libssl.so | grep SSL_library_init  
  // not found, since SSL_library_init() is deprecated in 1.1.1
   
openssl-1.0.2o: nm -gD libssl.so | grep SSL_library_init
0000000000055520 T SSL_library_init
```  

#### g. Update path of a dependency of an executable

```
ldd exec_file | grep libcurlpp
	libcurlpp.so.1 => not found

patchelf --replace-needed libcurlpp.so.1 /foo/bar/libcurlpp.so.1 exec_file
```

Another example (libcrypto.so.1.1 => not found):  

```
cd /opt/openssl-1.1.1f
ldd libssl.so
	linux-vdso.so.1 =>  (0x00007ffcaf911000)
	libcrypto.so.1.1 => not found
	libpthread.so.0 => /lib/x86_64-linux-gnu/libpthread.so.0 (0x00007fd4087b4000)
	libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007fd4083ea000)
	/lib64/ld-linux-x86-64.so.2 (0x00007fd408c65000)

sudo cp libcrypto.so.1.1 /lib/x86_64-linux-gnu/

ldd libssl.so
	linux-vdso.so.1 =>  (0x00007fff0f5d8000)
	libcrypto.so.1.0.0 => /lib/x86_64-linux-gnu/libcrypto.so.1.1 (0x00007fd3dcc1e000)
	libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007fd3dc854000)
	libdl.so.2 => /lib/x86_64-linux-gnu/libdl.so.2 (0x00007fd3dc650000)
	/lib64/ld-linux-x86-64.so.2 (0x00007fd3dd2d6000)

```

### Mac OS X

#### a. Show dependencies of executable or shared library:

Show dependencies of executable or shared library:

    $ otool -L robomongo

View LC_RPATH records of executable or shared library:

    $ otool -l robomongo | grep -C2 RPATH
    
(option `-C2` asks grep to output 2 context lines before and after the match)

For Robomongo binary it should be `$executable_path/../Frameworks`

More about `otool -l`

```				
$ otool -l /opt/robo/build/release/src/robomongo/Robo\ 3T.app/Contents/MacOS/Robo\ 3T 
....
Load command 23
          cmd LC_LOAD_DYLIB
      cmdsize 56
         name @rpath/lib/libssl.1.1.dylib (offset 24)
   time stamp 2 Thu Jan  1 03:00:02 1970
      current version 1.1.0
compatibility version 1.1.0
Load command 24
          cmd LC_LOAD_DYLIB
      cmdsize 56
         name @rpath/lib/libcrypto.1.1.dylib (offset 24)
   time stamp 2 Thu Jan  1 03:00:02 1970
      current version 1.1.0
compatibility version 1.1.0
....
Load command 27
          cmd LC_RPATH
      cmdsize 48
         path /opt/Qt5.9.3/5.9.3/clang_64/lib (offset 12)
Load command 28
          cmd LC_RPATH
      cmdsize 32
         path /opt/openssl-1.1.1f (offset 12)
```	 

#### b. Edit dependencies of executable or shared library:
```
// Before 
otool -L libssl.dylib 
libssl.dylib:
	/usr/local/lib/libssl.1.1.dylib (compatibility version 1.1.0, current version 1.1.0)
	/usr/local/lib/libcrypto.1.1.dylib (compatibility version 1.1.0, current version 1.1.0)
	/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1281.100.1)
    
// Edit
install_name_tool -id "@rpath/lib/libssl.1.1.dylib" libssl.dylib
install_name_tool -change /usr/local/lib/libcrypto.1.1.dylib @rpath/lib/libcrypto.1.1.dylib libssl.dylib

// After
otool -L libssl.dylib 
libssl.dylib:
	@rpath/lib/libssl.1.1.dylib (compatibility version 1.1.0, current version 1.1.0)
	@rpath/lib/libcrypto.1.1.dylib (compatibility version 1.1.0, current version 1.1.0)
	/usr/lib/libSystem.B.dylib (compatibility version 1.0.0, current version 1281.100.1)    
```    


Diagnostic of Qt plugins
------------------------

Run Robomongo in the following way to see how plugins are resolved:

    $ QT_DEBUG_PLUGINS=1 ./robomongo
    

