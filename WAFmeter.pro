# -------------------------------------------------
# Project created by QtCreator 2009-08-10T21:22:13
# -------------------------------------------------
TARGET = WAFmeter

include(opencv.pri)

# For Qt5
greaterThan(QT_MAJOR_VERSION, 4): {
    QT += widgets
    DEFINES += _QT5
}

# used for linking first with libs in /usr/local/lib before /usr/lib
QMAKE_LIBDIR_QT -= /usr/lib

TEMPLATE = app
#DEFINES += QT3_SUPPORT
CONFIG += qt \
    warn_on \
    debug_and_release

linux-* {
TMAKE_CXXFLAGS += -Wall \
    -g \
    -O2 \
    -fexceptions \
    -Wimplicit \
    -Wreturn-type \
    -Wunused \
    -Wswitch \
    -Wcomment \
    -Wuninitialized \
    -Wparentheses \
    -Wpointer-arith

    DEFINES += LINUX
    OBJECTS_DIR = .obj
    MOC_DIR = .moc
    UI_DIR = .ui
}

android* {
    message("============== ANDROID VERSION ==============")
    QT += network
    DEFINES += _ANDROID
    CONFIG += mobility
    MOBILITY =

    DEFINES += LINUX

    OBJECTS_DIR = .obj-android
    MOC_DIR = .moc-android
    UI_DIR = .ui-android
}

macx: {
    message("MacOS X specific options =================================================")
    ICON = icons/WAFMeter128.icns
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6
    OBJECTS_DIR = obj-macx
    MOC_DIR = moc-macx
    UI_DIR = ui-macx
}

# TARGET = $$join(TARGET,,,_debug)
# DEFINES += "TRANSLATION_DIR=\"Tamanoir.app/Contents/\""
linux-g++ { 
    message("Linux specific options =================================================")
    DEFINES += "TRANSLATION_DIR=/usr/share/"
}

win32:TARGET = $$join(TARGET,,d)

# }

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
RESOURCES += WAFmeter.qrc

# # INSTALLATION
# target.path = /usr/local/wafmeter
# INSTALLS += target
# FINAL CONFIGURATION ==================================================
TEMPLATE = app

message( "")
message( "")
message( "FINAL CONFIGURATION ================================================== ")
message( "Configuration : ")
message( " config : $$CONFIG ")
message( " defs : $$DEFINES ")
message( " libs : $$LIBS ")
message( "FINAL CONFIGURATION ================================================== ")
message( "")
message( "")

OTHER_FILES += qss/WAFMeter.qss \
    android/AndroidManifest.xml

DEPLOYMENTFOLDERS = # file1 dir1

include(deployment.pri)
qtcAddDeployment()

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android


