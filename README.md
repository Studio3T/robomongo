# End of Robo 3T Development

Robo 3T is no longer being developed by Studio 3T. Studio 3T recommends users looking for a MongoDB GUI client try [Studio 3T Free](https://studio3t.com/free), a free-forever edition of the Studio 3T tools.

Read more about the changes on the [Robo 3T Blog](https://blog.robomongo.org/studio3t-free/).

The last release of Robo 3T is version 1.4.4, downloadable from the following links: 

* [Robo 3T Windows .zip](https://download.studio3t.com/robomongo/windows/robo3t-1.4.4-windows-x86_64-e6ac9ec5.zip)

* [Robo 3T Windows .exe](https://download.studio3t.com/robomongo/windows/robo3t-1.4.4-windows-x86_64-e6ac9ec5.exe)

* [Robo 3T Mac](https://download.studio3t.com/robomongo/mac/robo3t-1.4.4-darwin-x86_64-e6ac9ec.dmg)

* [Robo 3T Linux](https://download.studio3t.com/robomongo/linux/robo3t-1.4.4-linux-x86_64-e6ac9ec.tar.gz)

And the source code from [Robo 3T repository](https://github.com/Studio3T/robomongo/latest).

Studio 3T would like to thank the Robo 3T community who used and supported the application, since it was acquired in 2017. This repository, the website and blog will be left online and available to maintain a record of what was one of the most influential MongoDB clients of its time.


About Robo 3T
===============

[Robo 3T](http://www.robomongo.org) (formerly Robomongo *) is a shell-centric cross-platform MongoDB management tool. Unlike most other MongoDB admin UI tools, Robo 3T embeds the actual `mongo` shell in a tabbed interface with access to a shell command line as well as GUI interaction.

The latest stable release **Robo 3T 1.4** embeds **MongoDB 4.2** shell.   

Blog:     http://blog.robomongo.org/robo-3t-1-4/  
Download: https://robomongo.org/download  
All Releases: https://github.com/Studio3T/robomongo/releases  
Watch: [Robo 3T Youtube channel](https://www.youtube.com/channel/UCM_7WAseRWeeiBikExppstA)  
Follow: https://twitter.com/Robomongo

**Embedded MongoDB shell history:**  
Robo 3T 1.4 -> MongoDB 4.2     
Robo 3T 1.3 -> MongoDB 4.0     
Robo 3T 1.1 -> MongoDB 3.4    
Robo 3T 0.9 -> MongoDB 3.2  
Robo 3T 0.8.x -> MongoDB 2.4.0  

\* [Robomongo has been acquired by 3T](https://studio3t.com/press/3t-software-labs-acquires-robomongo-the-most-widely-used-mongodb-tool/)

What's new in latest Robo 3T 1.4?
====================================

New Features:   
  - Mongo shell 4.2 upgrade  
  - Support for Ubuntu 20.04, macOS Big Sur and  macOS 10.15 (Catalina)   
  - SSH: ECDSA and Ed25519 keys support on Windows & macOS (issues #1719, #1530, #1590)  
  - Manually specify visible databases (issues #1696, #1368, #389)  
  - New Welcome Tab - embeds Chromium using QtWebEngine (Windows, macOS only)  
  - Import keys from old version: autoExpand, lineNumbers, debugMode and shellTimeoutSec  
   
Improvements:  
  - Qt Upgrade (v5.12.8 - Apr/2020, Windows & macOS only)  
  - OpenSSL upgrade (v1.1.1f - Mar/2020, Windows & macOS only)  
  - libssh2 upgrade (v1.9.0 - Jun/2019, Windows & macOS only)  
  - Database explorer section has smaller default width (#1556)
  - Remember database explorer section size   

Fixes:  
  - Fix previously broken IPv6 support from command line: robo3t --ipv6
  - Fix crash when paging used in tabbed result window (#1661)
  - Fix broken paging in DocumentDB (#1694)
  - Authentication database option isn't used properly (#1696)  
  - Add/Edit index ops fixed (re-written) (#1692)   
  - Crash when expanding admin users (#1728)   
  - Unable to run query after shell timeout reached (#1529)  
  - Fix broken F2, F3, F4 shortcuts for tabbed result view
  - One time re-order limit per new connections window to prevent data loss (macOS, #1790)  
  - Fix crash when new shell tab executed in server unreachable case  

Supported Platforms
===============

Note: This sections is for Robo 3T and it directly depends on what MongoDB suppports  
(See: https://docs.mongodb.com/manual/administration/production-notes/#prod-notes-supported-platforms)

| MongoDB Versions      | MongoDB Cloud Platforms |
| :-------------------- | :--------------------   | 
| 4.2                   | Mongo Atlas             |
| 4.0                   |
| 3.6                   |

| Windows                |   Mac                            | Linux                       |        
|:---------------------- | :--------------------------------| :---------------------------|
| Windows 64-bit 10      |  Mac OS X 11    (Big Sur)     	  | Linux Ubuntu 20.04 64-bit  |
  Windows 64-bit 8.1     |  Mac OS X 10.15 (Catalina)           | Linux Ubuntu 18.04 64-bit  |
| Windows 64-bit 7       |  Mac OS X 10.14 (Mojave)      |   |


Contribute!
===========

### Code Contributions

See all docs here: https://github.com/Studio3T/robomongo/wiki  

**Some important docs:**  
- [Build Diagram](https://github.com/Studio3T/robomongo/wiki/Robo-3T-Schematics:-Build,-Class-and-UI-Diagrams#1-build-diagram)
- [Static Code Analysis](https://github.com/Studio3T/robomongo/wiki/Static-Code-Analysis)
- [Robo 3T Feature Specisification](https://github.com/Studio3T/robomongo/wiki/Feature-Spec)
- [Debugging](https://github.com/Studio3T/robomongo/blob/master/docs/Debug.md)
- [Schematics](https://github.com/Studio3T/robomongo/tree/master/schematics)

Code contributions are always welcome! Just try to follow our pre-commit checks and coding style: 
- [Robo 3T Code Quality](https://github.com/paralect/robomongo/wiki/Robomongo-Code-Quality)
- [Robo 3T C++11/14 Transition Guide](https://github.com/Studio3T/robomongo/wiki/Robomongo-Cplusplus-11,-14-Transition-Guide)
- [Robo 3T Coding Style](https://github.com/paralect/robomongo/wiki/Robomongo-Coding-Style)

If you plan to contribute, please create a Github issue (or comment on the relevant existing issue) so we can help coordinate with upcoming release plans.

Pull requests (PRs) should generally be for discrete issues (i.e. one issue per PR please) and be clean to merge against the current master branch. It would also be helpful if you can confirm what testing has been done (specific O/S targets and MongoDB versions if applicable).

A usual naming approach for feature branches is `issue-###`. Include the issue number in your commit message / pull request description to link the PR to the original issue.

For example:
```#248: updated QScintilla to 2.4.8 for retina display support".```

### Testing

- [Unit-Tests](https://github.com/Studio3T/robomongo/wiki/Unit-Tests)  
- [Manual Tests](wiki/Tests)
- See all docs here: https://github.com/Studio3T/robomongo/wiki  

### Suggest Features

New feature suggestions or UI improvements are always welcome.
[Create a new feature request on github](https://github.com/paralect/robomongo/issues/new)

This project is powered by open source volunteers, so we have a limited amount of development resource to address all requests. We will certainly make best efforts to progress (particularly for those with strong community upvotes).


Download
========

You can download tested installer packages for macOS, Windows, and Linux from our site: [www.robomongo.org](http://www.robomongo.org).

Support
=======

Robo 3T is an open source project driven by volunteers. We'll try to get to your questions as soon as we can, but please be patient :).

You can:

 - [Create a new issue in the Github issue queue](https://github.com/paralect/robomongo/issues)

 - [Join developer discussion on Gitter](https://gitter.im/paralect/robomongo)


License
=======

Copyright 2014-2021 [3T Software Labs Ltd](https://studio3t.com/). All rights reserved.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3 as 
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
