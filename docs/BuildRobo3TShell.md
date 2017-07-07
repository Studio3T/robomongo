
Building Robo 3T Shell
========================

Robo 3T uses [modified version](https://github.com/paralect/robomongo-shell/tree/roboshell-v3.4) of MongoDB that we call Robo 3T Shell.  
Before you can build Robo 3T, you have to build Robo 3T Shell. The following instructions are applicable only for Mac OS X and Linux. For Windows instructions please check [Building Robo 3T on Windows](BuildRobo3TOnWindows.md).

Build for Mac OS X or Linux
---------------------------

#### Prerequisites

1. Install [Scons](http://scons.org/tag/releases.html) (2.3.5 or newer)


2. Clone Robo 3T Shell and checkout to `roboshell-v3.4` branch:

  ```sh
  $ git clone https://github.com/paralect/robomongo-shell.git
  $ cd robomongo-shell
  $ git checkout roboshell-v3.4
  ```

3. Build Robo 3T Shell:

  ```sh
  $ bin/build
  ```

Done! Now you can continue with building Robo 3T 
with embedded MongoDB 3.4 shell (that you've just built).


<br/>
#### Advanced helper commands

The following commands are needed only if you are planning to develop or deeper understand
Robo 3T or Robo 3T Shell build processes.

Clean build files for release mode (folder `build/opt` will be removed):

    $ bin/clean

Here is a command that is executed by `bin/build` script:

    $ scons mongo -j8 --release --osx-version-min=10.9
    
Argument `--osx-version-min` is required only for Mac OS.
    
Build MongoDB shell in debug mode:

    $ scons mongo -j8 --dbg

Clear builds:

    $ scons -c mongo        # clears release build
    $ scons -c --dbg mongo  # clears debug build
