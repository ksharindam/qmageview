HEADERS = invert.h
SOURCES = invert.cpp

TARGET  = $$qtLibraryTarget(invert)
DESTDIR = ..
INCLUDEPATH += ../../src

TEMPLATE        = lib
CONFIG         += plugin
QMAKE_CXXFLAGS  = -std=c++11
QMAKE_LFLAGS   += -s
LIBS           +=

QT += widgets

MOC_DIR =     ../../build
OBJECTS_DIR = ../../build

unix {
    INSTALLS += target
    target.path = /usr/local/share/photoquick/plugins
}

CONFIG -= debug_and_release debug
