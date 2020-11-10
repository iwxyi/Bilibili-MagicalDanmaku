QT       += core gui network websockets

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

RC_FILE += resource.rc

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

include($$PWD/qxtglobalshortcut5/qxt.pri)

INCLUDEPATH += \
    utils/ \
    live_danmaku/

SOURCES += \
    live_danmaku/livedanmakuwindow.cpp \
    main.cpp \
    mainwindow.cpp \
    taskwidget.cpp \
    utils/fileutil.cpp \
    utils/stringutil.cpp \

HEADERS += \
    live_danmaku/freecopyedit.h \
    live_danmaku/livedanmakuwindow.h \
    live_danmaku/livedanmaku.h \
    live_danmaku/portraitlabel.h \
    mainwindow.h \
    resource.rc \
    taskwidget.h \
    utils/fileutil.h \
    utils/netutil.h \
    utils/pinyinutil.h \
    utils/stringutil.h \
    ioapi.h \
    unzip.h \
    zconf.h \
    zip.h \
    zlib.h

FORMS += \
    mainwindow.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
        README.md \
        appicon.ico

LIBS += $$PWD/zlibstat.lib
