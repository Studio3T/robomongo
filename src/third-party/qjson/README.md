What is QJson
=============

QJson is a Qt-based library that maps JSON data to QVariant objects:
https://github.com/flavio/qjson

Robomongo statically links to this library.

QJson sources are provided in `sources` folder as-is, in their original form, 
without modifications. We should *not* put or modify any files in this folder.
All changes to original QJson, if they are inevitable, should be documented here.

CMake integration
=================

Original CMake listfiles are *not* used by Robomongo. Instead, we wrote our own 
CMakeLists.txt, located in this folder.


How to upgrade
==============

In order to upgrade to newer version of QJson you should:

1. Replace `sources` folder with newer version and 
2. Tweak `CMakeLists.txt` file, located in this folder.
3. Make sure that Robomongo compiles and works 
