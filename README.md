About Robomongo [![Join the chat at https://gitter.im/paralect/robomongo](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/paralect/robomongo?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)
===============

[Robomongo](http://www.robomongo.org) is a shell-centric cross-platform MongoDB management tool. Unlike most other MongoDB admin UI tools, Robomongo embeds the actual `mongo` shell in a tabbed interface with access to a shell command line as well as GUI interaction.

Starting from version 0.9, Robomongo is compatibile with MongoDB 3.x (including SCRAM-SHA-1 auth and support for WiredTiger storage engine). Robomongo 0.9 embeds the **MongoDB 3.2** shell.

Robomongo 0.8.x embeds the **MongoDB 2.4.0** shell.

What's new in latest Robomongo 1.0?
====================================
Latest release of Robomongo 1.0 includes: 
  - Update Notifications Bar
  - Welcome Tab
  - Welcome Wizard
  - Replica set name configurable on UI
  - Fixes to avoid same replica set name problem
  - Fixes to solve various problems regarding Data and Timestamp data types

Blog:     http://blog.robomongo.org/robomongo-1-0/  
Download: https://robomongo.org/download  
Watch: [Robomongo Youtube channel](https://www.youtube.com/channel/UCM_7WAseRWeeiBikExppstA)  
Follow: https://twitter.com/Robomongo

What's Planned for the Next Releases?
====================================

We are currently working towards next release [Robomongo 1.1](https://github.com/Studio3T/robomongo/projects/1).  
 
**Currently in progress:**
- [MongoDB 3.4 Support](https://github.com/Studio3T/robomongo/issues/1250)
- [New NumberDecimal (Decimal128) data type support](https://github.com/Studio3T/robomongo/issues/1248)
    
- [ECMAScript 2015 (Modernized JavaScript Implementation aka ES6) Support](https://github.com/Studio3T/robomongo/issues/1197)  
    http://www.ecma-international.org/ecma-262/6.0/index.html

- Security Improvements: OpenSSL version upgrade to openssl-1.0.1u (22-Sep-2016)
- Tool Chain Improvements: Modern C++14 features are enabled 

**Plans for Future:**
- Export/Import [High Vote] 
- User Roles [High Vote]
- Support execution of multiple simultaneous queries [#1161](https://github.com/paralect/robomongo/issues/1161)
- Selectable mongodb engines [#1249](https://github.com/paralect/robomongo/issues/1249)
- Enhancements for stability, running without crashes ([Stability Milestone](https://github.com/paralect/robomongo/milestone/15))
- More Documentation
- Unit Tests
- Refactoring Debug Log module

Supported Platforms
===============

| MongoDB Versions      |
| :-------------------- |
| 3.2                   |
| 3.0                   |
| 2.6                   |

| MongoDB Cloud Platforms|
| :------------ |
| MongoDB Atlas |
| Compose       |
| mLab          |
| ObjectRocket  |
| Amazon EC2    |

| Windows                |   Mac                            | Linux                       |        
|:---------------------- | :--------------------------------| :---------------------------|
| Windows 64-bit 10      |  Mac OS X 10.12 (Sierra)         | Linux Ubuntu 16.04 64-bit*  |
  Windows 64-bit 8.1     |  Mac OS X 10.11 (El Capitan)     | Linux Ubuntu 14.04 64-bit*  |
| Windows 64-bit 7       |  Mac OS X 10.10 (Yosemite)       | Linux CentOS 7 64-bit*      |
|                        |                                  | Linux CentOS 6 64-bit*      |

\* latest stable build


Download
========

You can download tested install packages for OS X, Windows, and Linux from our site: [www.robomongo.org](http://www.robomongo.org).

The latest stable release is currently [**Robomongo 1.0**](http://blog.robomongo.org/robomongo-1-0/).

Support
=======

Robomongo is an open source project driven by volunteers. We'll try to get to your questions as soon as we can, but please be patient :).

You can:

 - [Create a new issue in the Github issue queue](https://github.com/paralect/robomongo/issues)

 - [Join developer discussion on Gitter](https://gitter.im/paralect/robomongo)

Build
=====

The wiki contains prerequisites and instructions to [Build Robomongo](https://github.com/paralect/robomongo/wiki).

If you want to compile from source yourself, you should be able to do so cleanly from a [release branch](https://github.com/paralect/robomongo/releases).

Contribute!
===========

### Suggest Features

New feature suggestions or UI improvements are always welcome.
[Create a new feature request on github](https://github.com/paralect/robomongo/issues/new)

This project is powered by open source volunteers, so we have a limited amount of development resource to address all requests. We will certainly make best efforts to progress (particularly for those with strong community upvotes).

### Code Contributions

Code contributions are always welcome! Just try to follow our pre-commit checks and coding style: 
- [Robomongo Code Quality](https://github.com/paralect/robomongo/wiki/Robomongo-Code-Quality)
- [Robomongo Coding Style](https://github.com/paralect/robomongo/wiki/Robomongo-Coding-Style)

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

Copyright 2014-2017 [3T Software Labs Ltd](https://studio3t.com/). All rights reserved.

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3 as 
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
