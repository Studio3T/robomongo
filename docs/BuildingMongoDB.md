Building Robomongo Shell
========================

Robomongo uses [modified version](https://github.com/paralect/robomongo-shell/tree/roboshell-v3.2) of MongoDB that we call Robomongo Shell. Before you can build Robomongo, you have to build Robomongo Shell. The following instructions are applicable only for Mac OS X and Linux. For Windows instructions please check [Building Robomongo on Windows](BuildingRobomongoOnWindows.md)

Build for Mac OS X or Linux
---------------------------

1. Clone Robomongo Shell and checkout to `roboshell-v3.2` branch:

```sh
$ git clone https://github.com/paralect/robomongo-shell.git
$ cd robomongo-shell
$ git checkout roboshell-v3.2
```

2. Build Robomongo Shell:

```sh
$ bin/build
```

Done! Now you can continue with [Step 2](BuildingRobomongo.md#step-2) and build Robomongo 
with embedded MongoDB 3.2 shell.


#### Advanced helper commands

Clean build files for release mode:

    $ bin/clean

Here is a command that is executed by `bin/build` script:

    $ scons mongo -j8 --release --osx-version-min=10.9
    
Argument `--osx-version-min` is required only for Mac OS.
    
Build MongoDB shell in debug mode:

    $ scons mongo -j8 --dbg

Clear builds:

    $ scons -c mongo        # clears release build
    $ scons -c --dbg mongo  # clears debug build
