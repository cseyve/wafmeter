# -------------------------------------------------
# Project created by QtCreator 2009-08-10T21:22:13
# -------------------------------------------------
TARGET = WAFmeter
TEMPLATE = app
DEFINES += QT3_SUPPORT
include(opencv.pri)
SOURCES += src/main.cpp \
    src/wafmainwindow.cpp \
    src/wafmeter.cpp \
    src/imgutils.cpp
HEADERS += inc/wafmainwindow.h \
    inc/wafmeter.h \
    inc/imgutils.h
FORMS += ui/wafmainwindow.ui
INCLUDEPATH += inc
INCLUDEPATH += .
DEPENDPATH += inc
DEPENDPATH += .
RESOURCES += ui/WAFMeter.qrc
