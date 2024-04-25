QT += \
    core \
    gui \
    positioning \
    location \
    quickwidgets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    configs.cpp \
    ipdb.cpp \
    libmaxminddb\data-pool.c \
    libmaxminddb\maxminddb.c \
    nyatrace_gui.cpp \
    main.cpp \
    nyatrace_logs.cpp \
    nyatrace_window.cpp \
    resolve_core.cpp \
    tracing_core.cpp \
    tracing_utils.cpp \
    tracing_worker.cpp

HEADERS += \
    configs.h \
    data-pool.h \
    ipdb.h \
    ipdb_settings.h \
    libmaxminddb\data-pool.h \
    libmaxminddb\maxminddb-compat-util.h \
    libmaxminddb\maxminddb.h \
    libmaxminddb\maxminddb_config.h \
    mode.h \
    nyatrace_gui.h \
    nyatrace_logs.h \
    nyatrace_window.h \
    resolve_core.h \
    tracing_core.h \
    tracing_defs.h \
    tracing_utils.h \
    tracing_worker.h

FORMS += \
    nyatrace_gui.ui \
    nyatrace_logs.ui

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

VERSION = 0.1.10.0

DEFINES += \
    APP_VERSION=\\\"$$VERSION\\\"

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
