Decide the future of Robomongo
==============
![Robomongo mascot](http://app.robomongo.org/images/robomongo-64x64.png) 

We've started a campaign on [Indiegogo](https://www.indiegogo.com/projects/save-robomongo) in order to continue development of Robomongo. We want to keep Robomongo free forever. Everyone has a chance to decide the future of the project. Read more on [Robomongo website](http://robomongo.org).



About Robomongo
===============

[Robomongo](http://www.robomongo.org) is a shell-centric cross-platform MongoDB management tool. Unlike most other MongoDB admin UI tools, Robomongo embeds the actual `mongo` shell in a tabbed interface with access to a shell command line as well as GUI interaction.

Note: Robomongo 0.8.x currently embeds the **MongoDB 2.4.0** shell using the SpiderMonkey JavaScript engine. We are working on updating this to the MongoDB 3.0 shell with V8 for the 0.9.x release series.


Download
========

You can download tested install packages for OS X, Windows, and Linux from our site: [www.robomongo.org](http://www.robomongo.org).

The latest stable release is currently **Robomongo 0.8.5**.

Support
=======

Robomongo is an open source project driven by volunteers. We'll try to get to your questions as soon as we can, but please be patient :).

You can:

 - [Create a new issue in the Github issue queue](https://github.com/paralect/robomongo/issues)

 - [Join developer discussion on Gitter](https://gitter.im/paralect/robomongo) [![Join the chat at https://gitter.im/paralect/robomongo](https://badges.gitter.im/Join%20Chat.svg)](https://gitter.im/paralect/robomongo?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Build
=====

The wiki contains prerequisites and instructions to [Build Robomongo](https://github.com/paralect/robomongo/wiki/Build-Robomongo).

If you want to compile from source yourself, you should be able to do so cleanly from a [release branch](https://github.com/paralect/robomongo/releases).

What's Planned for the Next Release?
====================================

We are currently working toward a [0.9.0 release milestone](https://github.com/paralect/robomongo/milestones/Robomongo%200.9.0)
with some major features targeted including compatibility with MongoDB 3.0
(updated shell, SCRAM-SHA-1 auth, support for WiredTiger storage engine)
and full support for replica sets.

Contribute!
===========

### Vote for Features

Please view our [Magic Backlog](http://app.robomongo.org/backlog) to see features prioritised by donation. The small amount of donations we currently receive are put toward hosting costs.

### Suggest Features

New feature suggestions or UI improvements are always welcome.

This project is powered by open source volunteers, so we have a limited amount of development resource to address all requests. We will certainly make best efforts to progress (particularly for those with strong community upvotes).

### Code Contributions

Code contributions are always welcome! Just try to follow our coding style: [Robomongo Coding Style](https://github.com/paralect/robomongo/wiki/Robomongo-Coding-Style).

If you plan to contribute, please create a Github issue (or comment on the relevant existing issue) so we can help coordinate with upcoming release plans.

For a general workflow, see Github's guide to [Fork a Repo](https://help.github.com/articles/fork-a-repo/).

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

Copyright (C) 2013-2015 [Paralect, Inc](http://www.paralect.com)

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License version 3 as 
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
