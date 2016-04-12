What is QScintilla
==================

QScintilla is a port to Qt of Scintilla C++ editor:
https://riverbankcomputing.com/software/qscintilla/intro

Robomongo statically links to this library.

QScintilla sources are provided in `sources` folder as-is, in their original form, 
without modifications. We should *not* put or modify any files in this folder.
All changes to original QScintilla, if they are inevitable, should be documented here.

What was changed?
=================

To make builds faster (and only for this) we commented all lexers, except CPP, in file `src/Catalogue.cpp`:

```cpp
int Scintilla_LinkLexers() {
    ...
//	LINK_LEXER(lmA68k);
//	LINK_LEXER(lmAbaqus);
//	LINK_LEXER(lmAda);
    ...
	LINK_LEXER(lmCPP);
	...
}

```

We fixed problem of active caret when editor pane is not active. For this we
commented single line in `sources/Qt4Qt5/qsciscintillabase.cpp`, line 345:

```cpp
//    if (e->reason() == Qt::ActiveWindowFocusReason)
```

This problem was discussed at least here: https://github.com/openscad/openscad/issues/1176

CMake integration
=================

CMake build script is written according to provided qmake file:
`sources/Qt4Qt4/qscintilla.pro`


How to upgrade
==============

In order to upgrade to newer version of QScintilla you should:

1. Replace `sources` folder with newer version 
2. Tweak `CMakeLists.txt` file, located in this folder.
3. Make sure that Robomongo compiles and works 
