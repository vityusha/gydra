TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

VERSION = 0.1

DEFINES += VERSION=\\\"$$VERSION\\\"

INCLUDEPATH += win/portaudio/include

win32 {
    LIBS += -lWs2_32 -Lwin/portaudio/lib -lportaudio_x86
}
unix {
    LIBS += -lportaudio -lm -lasound
}

SOURCES += src/params.c \
    src/main.c \
    src/devs.c \
    src/audio.c \
    src/net.c

HEADERS += \
    src/params.h \
    src/devs.h \
    src/audio.h \
    src/net.h \
    src/protocol.h

