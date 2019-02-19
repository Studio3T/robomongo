About Robo 3T
===============

[Robo 3T](http://www.robomongo.org) (formerly Robomongo *) is a shell-centric cross-platform MongoDB management tool. Unlike most other MongoDB admin UI tools, Robo 3T embeds the actual `mongo` shell in a tabbed interface with access to a shell command line as well as GUI interaction.

Starting from version 1.1, Robo 3T embeds the **MongoDB 3.4** shell.

Starting from version 0.9, Robo 3T is compatibile with MongoDB 3.x (including SCRAM-SHA-1 auth and support for WiredTiger storage engine). Robo 3T 0.9 embeds the **MongoDB 3.2** shell.

Robo 3T 0.8.x embeds the **MongoDB 2.4.0** shell.

\* [Robomongo has been acquired by 3T](https://studio3t.com/press/3t-software-labs-acquires-robomongo-the-most-widely-used-mongodb-tool/)

What's new in latest Robo 3T 1.2?
====================================

Fixes:  
  - Aggregate query results are not pagaeble [#1058](https://github.com/Studio3T/robomongo/issues/1058)      
  - Big double numbers are showing incorrectly [#1447](https://github.com/Studio3T/robomongo/issues/1447)  
  - Editing a document from a projection causes data loss (override) [#881](https://github.com/Studio3T/robomongo/issues/881)
  - SSH: Resources temporarily unavailable. Error when starting up SSH session [#1189](https://github.com/Studio3T/robomongo/issues/1189#issuecomment-353279070)
  - robo3t 1.1 failed to start on Ubuntu 17.04 [#1385](https://github.com/Studio3T/robomongo/issues/1385)
  - Robo 3T is not opening on MacOS High Sierra [#1412](https://github.com/Studio3T/robomongo/issues/1412#issuecomment-353276002)
  - Cannot load list of indexes [#1454](https://github.com/Studio3T/robomongo/issues/1454)
  - Robo 3T is not buildable on latest MacOS High Sierra  
  
Improvements 
  - Qt version upgraded from Qt 5.7.0 to Qt 5.9.3
  - Auto switch to HTTP from HTTPS if the program crashes due to HTTPS related operations 
  - Improvements for Robo 3T is crashing constantly on High Sierra [#1412](https://github.com/Studio3T/robomongo/issues/1412#issuecomment-353276002)
  - PR: Improved query time formatting (#1358) [#1455](https://github.com/Studio3T/robomongo/issues/1455)

Blog:     http://blog.robomongo.org/robo-3t-1-2/  
Download: https://robomongo.org/download  
Watch: [Robo 3T Youtube channel](https://www.youtube.com/channel/UCM_7WAseRWeeiBikExppstA)  
Follow: https://twitter.com/Robomongo

Supported Platforms
===============

Note: This sections is for Robo 3T and it directly depends on what MongoDB suppports  
(See: https://docs.mongodb.com/manual/administration/production-notes/#prod-notes-supported-platforms)

| MongoDB Versions      |
| :-------------------- |
| 3.4                   |
| 3.2                   |
| 3.0                   |
| 2.6                   |

| MongoDB Cloud Platforms|
| :------------ |
| MongoDB Atlas |
| Compose       |
| mLab          |
| ObjectRocket  | 
| ScaleGrid     |
| Amazon EC2    |

| Windows                |   Mac                            | Linux                       |        
|:---------------------- | :--------------------------------| :---------------------------|
| Windows 64-bit 10      |  Mac OS X 10.13 (High Sierra)    | Linux Ubuntu 17.10 64-bit*  |
  Windows 64-bit 8.1     |  Mac OS X 10.12 (Sierra)         | Linux Ubuntu 16.04 64-bit*  |
| Windows 64-bit 7       |  Mac OS X 10.11 (El Capitan)     | Linux CentOS 7 64-bit* **   |
|                        |                                  | Linux CentOS 6 64-bit*  **  |

\* Latest stable build  
\** Support for CentOS temporarily dropped starting from version 1.1  
\*** Windows install requires Visual C++ redistributable 2008 [https://www.microsoft.com/en-us/download/details.aspx?id=29](download)

Download
========

You can download tested install packages for OS X, Windows, and Linux from our site: [www.robomongo.org](http://www.robomongo.org).

The latest stable release is currently [**Robo 3T 1.2**](http://blog.robomongo.org/robo-3t-1-2/).

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

Copyright 2014-2018 [3T Software Labs Ltd](https://studio3t.com/). All rights reserved.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3 as 
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
