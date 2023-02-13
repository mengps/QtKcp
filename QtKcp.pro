QT -= gui
QT += network

TEMPLATE = lib
DEFINES += QTKCP_LIBRARY

DESTDIR = $$PWD/buildlib

TARGET = $$qtLibraryTarget(QtKcp)

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    src/ikcp.c \
    src/qkcpserver.cpp \
    src/qkcpsocket.cpp

HEADERS += \
    src/ikcp.h \
    src/qkcpserver.h \
    src/qkcpsocket_global.h \
    src/qkcpsocket.h \
    src/qkcpsocket_p.h

# Default rules for deployment.
unix {
    target.path = /usr/lib
}
!isEmpty(target.path): INSTALLS += target
