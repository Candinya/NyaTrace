QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    data-pool.c \
    maxminddb.c \
    tr_thread.cpp \
    tr_gui.cpp \
    main.cpp \
    tr_utils.cpp

HEADERS += \
    data-pool.h \
    maxminddb-compat-util.h \
    maxminddb.h \
    maxminddb_config.h \
    mmdb_settings.h \
    tr_thread.h \
    tr_gui.h \
    tr_utils.h

FORMS += \
    tr_gui.ui

LIBS += \
    -lws2_32

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
