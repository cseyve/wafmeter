# -------------------------------------------------
# Project created by QtCreator 2009-08-10T21:22:13
# -------------------------------------------------
TARGET = WAFmeter

include(opencv.pri)

TEMPLATE = app
#DEFINES += QT3_SUPPORT
CONFIG += qt \
    warn_on \
    debug_and_release

linux-g++:TMAKE_CXXFLAGS += -Wall \
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

macx: {
    message("MacOS X specific options =================================================")
    ICON = icons/WAFMeter128.icns
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.6
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
OTHER_FILES += qss/WAFMeter.qss
