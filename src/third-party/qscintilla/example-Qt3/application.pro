TEMPLATE	= app
TARGET		= application

CONFIG		+= qt warn_on release

LIBS		+= -lqscintilla2

HEADERS		= application.h
SOURCES		= application.cpp \
		  main.cpp
