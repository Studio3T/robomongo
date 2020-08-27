About Robo 3T
===============

[Robo 3T](http://www.robomongo.org) (formerly Robomongo *) is a shell-centric cross-platform MongoDB management tool. Unlike most other MongoDB admin UI tools, Robo 3T embeds the actual `mongo` shell in a tabbed interface with access to a shell command line as well as GUI interaction.

Latest Robo 3T 1.4 embeds **MongoDB 4.2** shell.

**MongoDB shell history:**   
Robo 3T 1.3: **MongoDB 4.0** shell.  
Robo 3T 1.1: **MongoDB 3.4** shell.  
Robo 3T 0.9: **MongoDB 3.2** shell (including SCRAM-SHA-1 auth and support for WiredTiger storage engine).  
Robo 3T 0.8.x: **MongoDB 2.4.0** shell  

\* [Robomongo has been acquired by 3T](https://studio3t.com/press/3t-software-labs-acquires-robomongo-the-most-widely-used-mongodb-tool/)

What's new in latest Robo 3T 1.4?
====================================

New Features:   
  - Mongo shell 4.2 upgrade  
  - Support for Ubuntu 20.04 and macOS 10.15 (Catalina)   
  - SSH: ECDSA and Ed25519 keys support on Windows & macOS (issues #1719, #1530, #1590)  
  - Manually specify visible databases (issues #1696, #1368, #389)  
   
Improvements:  
  - Qt Upgrade (v5.12.8 - Apr/2020, Windows & macOS only)  
  - OpenSSL upgrade (v1.1.1f - Mar/2020, Windows & macOS only)  
  - libssh2 upgrade (v1.9.0 - Jun/2019, Windows & macOS only)  

Fixes:  
  - Authentication database option isn't used properly (#1696)  
  - Add/Edit index ops fixed (re-written) (#1692)   
  - Crash when expanding admin users (#1728)   
  - IPv6 support (previously broken) restored   
  - Unable to run query after shell timeout reached (#1529)  

Blog:     http://blog.robomongo.org/robo-3t-1-4/  
Download: https://robomongo.org/download  
Watch: [Robo 3T Youtube channel](https://www.youtube.com/channel/UCM_7WAseRWeeiBikExppstA)  
Follow: https://twitter.com/Robomongo

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
| Windows 64-bit 10      |  Mac OS X 10.15 (Catalina)     	  | Linux Ubuntu 20.04 64-bit*  |
  Windows 64-bit 8.1     |  Mac OS X 10.14 (Mojave)           | Linux Ubuntu 18.04 64-bit*  |
| Windows 64-bit 7       |  Mac OS X 10.13 (High Sierra)      | Linux CentOS 7 64-bit* **   |
|                        |                                    | Linux CentOS 6 64-bit*  **  |

\* Latest stable build  
\** Support for CentOS temporarily dropped starting from version 1.1  

Download
========

You can download tested installer packages for macOS, Windows, and Linux from our site: [www.robomongo.org](http://www.robomongo.org).

The latest stable release is currently [**Robo 3T 1.4**](http://blog.robomongo.org/robo-3t-1-4/).

Support
=======

Robo 3T is an open source project driven by volunteers. We'll try to get to your questions as soon as we can, but please be patient :).

You can:

 - [Create a new issue in the Github issue queue](https://github.com/paralect/robomongo/issues)

 - [Join developer discussion on Gitter](https://gitter.im/paralect/robomongo)

Contribute!
===========

### Suggest Features

New feature suggestions or UI improvements are always welcome.
[Create a new feature request on github](https://github.com/paralect/robomongo/issues/new)

This project is powered by open source volunteers, so we have a limited amount of development resource to address all requests. We will certainly make best efforts to progress (particularly for those with strong community upvotes).

### Code Contributions

Code contributions are always welcome! Just try to follow our pre-commit checks and coding style: 
- [Robo 3T Code Quality](https://github.com/paralect/robomongo/wiki/Robomongo-Code-Quality)
- [Robo 3T C++11/14 Transition Guide](https://github.com/Studio3T/robomongo/wiki/Robomongo-Cplusplus-11,-14-Transition-Guide)
- [Robo 3T Coding Style](https://github.com/paralect/robomongo/wiki/Robomongo-Coding-Style)

If you plan to contribute, please create a Github issue (or comment on the relevant existing issue) so we can help coordinate with upcoming release plans.

Pull requests (PRs) should generally be for discrete issues (i.e. one issue per PR please) and be clean to merge against the current master branch. It would also be helpful if you can confirm what testing has been done (specific O/S targets and MongoDB versions if applicable).

A usual naming approach for feature branches is `issue-###`. Include the issue number in your commit message / pull request description to link the PR to the original issue.

For example:
```#248: updated QScintilla to 2.4.8 for retina display support".```

### Design

There are some cosmetic issues we could use help with (designing images or UI). They are marked in the issue queue with a [`Cosmetic`](https://github.com/paralect/robomongo/labels/cosmetic) label. If you see an open issue that you'd like to contribute to, please feel free to volunteer by commenting on it.

### Testing

There are a number of issues we could use help with reproducing. They are marked in the issue queue with a [`Needs Repro`](https://github.com/paralect/robomongo/labels/needs%20repro) label. If you see an open issue that doesn't appear to be reproducible yet, please feel free to test and comment with your findings.


License
=======

Copyright 2014-2020 [3T Software Labs Ltd](https://studio3t.com/). All rights reserved.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3 as 
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
