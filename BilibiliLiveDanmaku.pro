QT       += core gui network websockets multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS QT_MESSAGELOGCONTEXT

RC_FILE += resource.rc

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

include($$PWD/qxtglobalshortcut5/qxt.pri)

INCLUDEPATH += \
    utils/ \
    live_danmaku/ \
    interactive_buttons/ \
    facile_menu/ \
    order_player/ \
    color_octree/

SOURCES += \
    color_octree/coloroctree.cpp \
    color_octree/imageutil.cpp \
    facile_menu/facilemenu.cpp \
    facile_menu/facilemenuitem.cpp \
    interactive_buttons/interactivebuttonbase.cpp \
    live_danmaku/livedanmakuwindow.cpp \
    main.cpp \
    mainwindow.cpp \
    order_player/desktoplyricwidget.cpp \
    order_player/numberanimation.cpp \
    order_player/orderplayerwindow.cpp \
    taskwidget.cpp \
    utils/fileutil.cpp \
    utils/stringutil.cpp \
    videolyricscreator.cpp

HEADERS += \
    color_octree/coloroctree.h \
    color_octree/imageutil.h \
    facile_menu/facilemenu.h \
    facile_menu/facilemenuitem.h \
    interactive_buttons/interactivebuttonbase.h \
    live_danmaku/commonvalues.h \
    live_danmaku/freecopyedit.h \
    live_danmaku/livedanmakuwindow.h \
    live_danmaku/livedanmaku.h \
    live_danmaku/portraitlabel.h \
    mainwindow.h \
    order_player/desktoplyricwidget.h \
    order_player/lyricstreamwidget.h \
    order_player/numberanimation.h \
    order_player/orderplayerwindow.h \
    order_player/songbeans.h \
    taskwidget.h \
    utils/dlog.h \
    utils/fileutil.h \
    utils/netutil.h \
    utils/pinyinutil.h \
    utils/stringutil.h \
    videolyricscreator.h

FORMS += \
    mainwindow.ui \
    order_player/orderplayerwindow.ui \
    videolyricscreator.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
        README.md \
        appicon.ico \
        resource.rc \

RESOURCES += \
    resource.qrc
