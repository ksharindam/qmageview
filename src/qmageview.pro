######################################################################
# Automatically generated by qmake (2.01a) Sun Oct 29 20:36:57 2017
######################################################################

TEMPLATE = app
TARGET = qmageview
DEPENDPATH += .
INCLUDEPATH += .
QMAKE_CXXFLAGS = -fopenmp -std=c++11
LIBS += -lgomp

CONFIG -= debug

# build dir
MOC_DIR = build
RCC_DIR = build
UI_DIR = build
OBJECTS_DIR = build
mytarget.commands += $${QMAKE_MKDIR} build

# Input
HEADERS += canvas.h main.h photogrid.h dialogs.h
SOURCES += common.cpp exif.cpp canvas.cpp main.cpp photogrid.cpp dialogs.cpp filters.cpp pdfwriter.cpp
RESOURCES += resources.qrc
FORMS += mainwindow.ui resize_dialog.ui photogrid_dialog.ui gridsetup_dialog.ui \
         collage_dialog.ui collagesetup_dialog.ui

# install
!win32 {
    INSTALLS += target
    target.path = /usr/local/bin
}

