QT       += core gui positioning location quickwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    data-pool.c \
    ipdb.cpp \
    maxminddb.c \
    nyatrace_gui.cpp \
    main.cpp \
    tracing_core.cpp

HEADERS += \
    data-pool.h \
    ipdb.h \
    maxminddb-compat-util.h \
    maxminddb.h \
    maxminddb_config.h \
    mmdb_settings.h \
    nyatrace_gui.h \
    tracing_core.h \
    tracing_defs.h

FORMS += \
    nyatrace_gui.ui

LIBS += \
    -lws2_32 \
    -liphlpapi

DEFINES += \
    _WINSOCKAPI_

RESOURCES += \
    tracing_map.qml

RC_ICONS += \
    icon.ico

TARGET = NyaTrace

VERSION = 0.1.7.1

DEFINES += \
    APP_VERSION=\\\"$$VERSION\\\"

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
