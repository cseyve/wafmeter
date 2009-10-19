#-------------------------------------------------
#
# Project created by QtCreator 2009-09-03T08:30:46
#
#-------------------------------------------------

QT       += qt3support

QT       -= gui

TARGET = wafconsole
CONFIG   += console
CONFIG   -= app_bundle

include(../opencv.pri)

TEMPLATE = app

INCLUDEPATH += ../inc
DEPENDPATH += ../inc


HEADERS += ../inc/wafmeter.h \
	../inc/imgutils.h

SOURCES += main.cpp \
	../src/imgutils.cpp \
	../src/wafmeter.cpp




