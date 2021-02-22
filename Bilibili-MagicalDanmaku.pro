QT       += core gui network websockets multimedia multimediawidgets texttospeech

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

contains(DEFINES,ANDROID){
    message("shortcuts not support")
}else{
    include($$PWD/qxtglobalshortcut5/qxt.pri)
}

INCLUDEPATH += \
    utils/ \
    live_danmaku/ \
    interactive_buttons/ \
    facile_menu/ \
    order_player/ \
    color_octree/ \
    lucky_draw/ \
    video_player/ \
    widgets/ \
    gif/ \
    picture_browser/

SOURCES += \
    color_octree/coloroctree.cpp \
    color_octree/imageutil.cpp \
    facile_menu/facilemenu.cpp \
    facile_menu/facilemenuitem.cpp \
    gif/avilib.cpp \
    gif/gif.cpp \
    interactive_buttons/interactivebuttonbase.cpp \
    live_danmaku/livedanmakuwindow.cpp \
    lucky_draw/luckydrawwindow.cpp \
    main.cpp \
    mainwindow.cpp \
    order_player/desktoplyricwidget.cpp \
    order_player/logindialog.cpp \
    order_player/numberanimation.cpp \
    order_player/orderplayerwindow.cpp \
    picture_browser/picturebrowser.cpp \
    picture_browser/resizablepicture.cpp \
    server.cpp \
    utils/xfytts.cpp \
    video_player/videosurface.cpp \
    widgets/catchyouwidget.cpp \
    widgets/conditioneditor.cpp \
    widgets/eternalblockdialog.cpp \
    widgets/eventwidget.cpp \
    widgets/fluentbutton.cpp \
    widgets/mytabwidget.cpp \
    widgets/replywidget.cpp \
    widgets/roomstatusdialog.cpp \
    widgets/taskwidget.cpp \
    utils/fileutil.cpp \
    utils/stringutil.cpp \
    utils/textinputdialog.cpp \
    video_player/livevideoplayer.cpp \
    widgets/videolyricscreator.cpp

HEADERS += \
    color_octree/coloroctree.h \
    color_octree/imageutil.h \
    facile_menu/facilemenu.h \
    facile_menu/facilemenuitem.h \
    gif/avilib.h \
    gif/gif.h \
    interactive_buttons/interactivebuttonbase.h \
    live_danmaku/commonvalues.h \
    live_danmaku/freecopyedit.h \
    live_danmaku/livedanmakuwindow.h \
    live_danmaku/livedanmaku.h \
    live_danmaku/portraitlabel.h \
    lucky_draw/luckydrawwindow.h \
    mainwindow.h \
    order_player/clickslider.h \
    order_player/desktoplyricwidget.h \
    order_player/itemselectionlistview.h \
    order_player/logindialog.h \
    order_player/lyricstreamwidget.h \
    order_player/numberanimation.h \
    order_player/orderplayerwindow.h \
    order_player/roundedpixmaplabel.h \
    order_player/songbeans.h \
    picture_browser/ASCII_Art.h \
    picture_browser/picturebrowser.h \
    picture_browser/resizablepicture.h \
    qhttpserver/qhttpconnection.h \
    qhttpserver/qhttprequest.h \
    qhttpserver/qhttpresponse.h \
    qhttpserver/qhttpserver.h \
    qhttpserver/qhttpserverapi.h \
    qhttpserver/qhttpserverfwd.h \
    utils/xfytts.h \
    video_player/videosurface.h \
    widgets/RoundedAnimationLabel.h \
    widgets/catchyouwidget.h \
    widgets/conditioneditor.h \
    widgets/eternalblockdialog.h \
    widgets/eventwidget.h \
    widgets/fluentbutton.h \
    widgets/mytabwidget.h \
    widgets/replywidget.h \
    widgets/roomstatusdialog.h \
    widgets/taskwidget.h \
    utils/dlog.h \
    utils/fileutil.h \
    utils/netutil.h \
    utils/pinyinutil.h \
    utils/stringutil.h \
    utils/textinputdialog.h \
    video_player/livevideoplayer.h \
    widgets/videolyricscreator.h

FORMS += \
    lucky_draw/luckydrawwindow.ui \
    mainwindow.ui \
    order_player/logindialog.ui \
    order_player/orderplayerwindow.ui \
    picture_browser/picturebrowser.ui \
    utils/textinputdialog.ui \
    video_player/livevideoplayer.ui \
    widgets/catchyouwidget.ui \
    widgets/eternalblockdialog.ui \
    widgets/roomstatusdialog.ui \
    widgets/videolyricscreator.ui

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    README.md \
    android/AndroidManifest.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat \
    android/res/values/libs.xml \
    appicon.ico \
    pictures/神奇弹幕-用法.pdf \
    resource.rc \
    resources/LAVFilters-0.74.1-Installer.exe \
    resources/点歌姬-用法.pdf \
    resources/神奇弹幕-用法.pdf

RESOURCES += \
    resource.qrc

contains(ANDROID_TARGET_ARCH,) {
    ANDROID_PACKAGE_SOURCE_DIR = \
        $$PWD/android
}


unix|win32: LIBS += -L$$PWD/libs/ -lqhttpserver -lversion

INCLUDEPATH += $$PWD/libs \
    qhttpserver/
DEPENDPATH += $$PWD/libs
