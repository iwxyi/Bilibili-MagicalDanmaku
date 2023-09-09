QT       += core gui network websockets multimedia multimediawidgets sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

#TARGET = MagicalDanmaku

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS QT_MESSAGELOGCONTEXT HAVE_CONFIG_H
DEFINES += ENABLE_TEXTTOSPEECH
#DEFINES += ZUOQI_ENTRANCE

RC_FILE += resources/resource.rc
unix {
    ICON = resources/icons/appicon.ico
}

# 图片太大，会导致 cc1plus.exe:-1: error: out of memory allocating 4198399 bytes 错误
CONFIG += resources_big

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
# DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

win32{
    DEFINES += ENABLE_SHORTCUT ENABLE_TRAY
}
DEFINES += ENABLE_HTTP_SERVER
#unix:!macx{
#    DEFINES += ENABLE_SHORTCUT ENABLE_TRAY
#}
#unix:!android{
#    DEFINES += ENABLE_SHORTCUT
#}

contains(DEFINES, ENABLE_SHORTCUT) {
    include($$PWD/third_party/qxtglobalshortcut5/qxt.pri)
}else{
    # message("module: shortcuts disabled")
}

contains(DEFINES, ENABLE_TEXTTOSPEECH) {
    QT += texttospeech
}

# 调试
# release版本可调试
# QMAKE_CXXFLAGS_RELEASE #+= $$QMAKE_CFLAGS_RELEASE_WITH_DEBUGINFO
# release版也将生成“.pdb”后缀的调试信息文件
# 等同于开启 Generate separate debug info
# QMAKE_LFLAGS_RELEASE = /INCREMENTAL:NO /DEBUG # 这个报错！
# 调用库
# LIBS += -lDbgHelp


INCLUDEPATH += \
    global/ \
    mainwindow/ \
    services/ \
    services/live_services/ \
    services/entities/ \
    services/code_runner/ \
    services/sql_service/ \
    third_party/utils/ \
    mainwindow/list_items/ \
    mainwindow/live_danmaku/ \
    third_party/interactive_buttons/ \
    third_party/facile_menu/ \
    order_player/ \
    third_party/color_octree/ \
    widgets/lucky_draw/ \
    widgets/video_player/ \
    widgets/catch_you_dialog/ \
    widgets/eternal_block_dialog/ \
    widgets/login_dialog/ \
    widgets/room_status_dialog/ \
    widgets/video_lyric_creator/ \
    widgets/guard_online/ \
    widgets/smooth_scroll/ \
    widgets/buy_vip/ \
    widgets/ \
    third_party/ \
    widgets/editor/ \
    widgets/db_browser/ \
    third_party/gif/ \
    third_party/picture_browser/ \
    third_party/notification/ \
    third_party/linear_check_box/ \
    third_party/mfaudioendpointcontrol_fixed/ \
    third_party/m3u8_downloader/

SOURCES += \
    mainwindow/run_cmd.cpp \
    services/chat_service/chatservice.cpp \
    services/code_runner/chatgptmanager.cpp \
    services/code_runner/coderunner.cpp \
    services/live_services/bili_livecmds.cpp \
    services/live_services/bili_liveopen_cmds.cpp \
    services/live_services/bili_liveopenservice.cpp \
    services/live_services/bili_liveservice.cpp \
    services/live_services/liveroomservice.cpp \
    services/live_services/livestatisticservice.cpp \
    services/sql_service/sqlservice.cpp \
    order_player/importsongsdialog.cpp \
    services/voice_service/voiceservice.cpp \
    services/web_server/webserver.cpp \
    third_party/color_octree/coloroctree.cpp \
    third_party/color_octree/coloroctreeutil.cpp \
    third_party/facile_menu/facilemenu.cpp \
    third_party/facile_menu/facilemenuitem.cpp \
    third_party/gif/avilib.cpp \
    third_party/gif/gif.cpp \
    third_party/interactive_buttons/appendbutton.cpp \
    third_party/interactive_buttons/generalbuttoninterface.cpp \
    third_party/interactive_buttons/infobutton.cpp \
    third_party/interactive_buttons/interactivebuttonbase.cpp \
    mainwindow/list_items/listiteminterface.cpp \
    mainwindow/live_danmaku/livedanmakuwindow.cpp \
    third_party/interactive_buttons/pointmenubutton.cpp \
    third_party/interactive_buttons/threedimenbutton.cpp \
    third_party/interactive_buttons/watercirclebutton.cpp \
    third_party/interactive_buttons/waterfallbuttongroup.cpp \
    third_party/interactive_buttons/waterfloatbutton.cpp \
    third_party/interactive_buttons/waterzoombutton.cpp \
    third_party/interactive_buttons/winclosebutton.cpp \
    third_party/interactive_buttons/winmaxbutton.cpp \
    third_party/interactive_buttons/winmenubutton.cpp \
    third_party/interactive_buttons/winminbutton.cpp \
    third_party/interactive_buttons/winrestorebutton.cpp \
    third_party/interactive_buttons/winsidebarbutton.cpp \
    third_party/linear_check_box/anicheckbox.cpp \
    third_party/linear_check_box/checkbox1.cpp \
    third_party/m3u8_downloader/m3u8downloader.cpp \
    third_party/notification/tipbox.cpp \
    third_party/notification/tipcard.cpp \
    third_party/qss_editor/qsseditdialog.cpp \
    third_party/qss_editor/qsshighlighteditor.cpp \
    third_party/utils/httpuploader.cpp \
    third_party/utils/microsofttts.cpp \
    third_party/utils/warmwishtutil.cpp \
    widgets/buy_vip/buyvipdialog.cpp \
    widgets/csvviewer.cpp \
    widgets/db_browser/dbbrowser.cpp \
    widgets/db_browser/sqleditor.cpp \
    widgets/guard_online/guardonlinedialog.cpp \
    widgets/lucky_draw/luckydrawwindow.cpp \
    mainwindow/main.cpp \
    mainwindow/mainwindow.cpp \
    order_player/desktoplyricwidget.cpp \
    order_player/logindialog.cpp \
    order_player/numberanimation.cpp \
    order_player/orderplayerwindow.cpp \
    third_party/picture_browser/picturebrowser.cpp \
    third_party/picture_browser/resizablepicture.cpp \
    third_party/qrencode/bitstream.c \
    third_party/qrencode/mask.c \
    third_party/qrencode/mmask.c \
    third_party/qrencode/mqrspec.c \
    third_party/qrencode/qrencode.c \
    third_party/qrencode/qrinput.c \
    third_party/qrencode/qrspec.c \
    third_party/qrencode/rsecc.c \
    third_party/qrencode/split.c \
    mainwindow/server.cpp \
    third_party/utils/xfytts.cpp \
    widgets/singleentrance.cpp \
    widgets/smooth_scroll/smoothlistwidget.cpp \
    widgets/smooth_scroll/waterfallscrollarea.cpp \
    widgets/variantviewer.cpp \
    widgets/video_player/videosurface.cpp \
    widgets/catch_you_dialog/catchyouwidget.cpp \
    widgets/editor/conditioneditor.cpp \
    widgets/escape_dialog/escapedialog.cpp \
    widgets/escape_dialog/hoverbutton.cpp \
    widgets/eternal_block_dialog/eternalblockdialog.cpp \
    mainwindow/list_items/eventwidget.cpp \
    widgets/fluentbutton.cpp \
    widgets/mytabwidget.cpp \
    widgets/login_dialog/qrcodelogindialog.cpp \
    mainwindow/list_items/replywidget.cpp \
    widgets/room_status_dialog/roomstatusdialog.cpp \
    mainwindow/list_items/taskwidget.cpp \
    third_party/utils/fileutil.cpp \
    third_party/utils/stringutil.cpp \
    third_party/utils/textinputdialog.cpp \
    widgets/video_player/livevideoplayer.cpp \
    widgets/video_lyric_creator/videolyricscreator.cpp

HEADERS += \
    global/accountinfo.h \
    global/platforminfo.h \
    global/runtimeinfo.h \
    global/usersettings.h \
    services/chat_service/chatservice.h \
    services/code_runner/chatgptmanager.h \
    services/code_runner/coderunner.h \
    services/entities/emoticon.h \
    services/entities/entities.h \
    services/live_services/bili_liveopenservice.h \
    services/live_services/bili_liveservice.h \
    services/live_services/liveroomservice.h \
    services/live_services/livestatisticservice.h \
    services/sql_service/sqlservice.h \
    order_player/importsongsdialog.h \
    services/voice_service/voiceservice.h \
    services/web_server/webserver.h \
    third_party/calculator/Digit.h \
    third_party/calculator/calculator_util.h \
    third_party/calculator/func_define.h \
    third_party/calculator/function.h \
    third_party/color_octree/coloroctree.h \
    third_party/color_octree/coloroctreeutil.h \
    third_party/facile_menu/facilemenu.h \
    third_party/facile_menu/facilemenuitem.h \
    third_party/gif/avilib.h \
    third_party/gif/gif.h \
    third_party/interactive_buttons/appendbutton.h \
    third_party/interactive_buttons/generalbuttoninterface.h \
    third_party/interactive_buttons/infobutton.h \
    third_party/interactive_buttons/interactivebuttonbase.h \
    mainwindow/list_items/listiteminterface.h \
    mainwindow/live_danmaku/freecopyedit.h \
    mainwindow/live_danmaku/livedanmakuwindow.h \
    services/entities/livedanmaku.h \
    mainwindow/live_danmaku/portraitlabel.h \
    third_party/interactive_buttons/pointmenubutton.h \
    third_party/interactive_buttons/threedimenbutton.h \
    third_party/interactive_buttons/watercirclebutton.h \
    third_party/interactive_buttons/waterfallbuttongroup.h \
    third_party/interactive_buttons/waterfloatbutton.h \
    third_party/interactive_buttons/waterzoombutton.h \
    third_party/interactive_buttons/winclosebutton.h \
    third_party/interactive_buttons/winmaxbutton.h \
    third_party/interactive_buttons/winmenubutton.h \
    third_party/interactive_buttons/winminbutton.h \
    third_party/interactive_buttons/winrestorebutton.h \
    third_party/interactive_buttons/winsidebarbutton.h \
    third_party/linear_check_box/anicheckbox.h \
    third_party/linear_check_box/checkbox1.h \
    third_party/m3u8_downloader/m3u8downloader.h \
    third_party/notification/notificationentry.h \
    third_party/notification/tipbox.h \
    third_party/notification/tipcard.h \
    third_party/qss_editor/qsseditdialog.h \
    third_party/qss_editor/qsshighlighteditor.h \
    third_party/utils/ImageSimilarityUtil.h \
    third_party/utils/bili_api_util.h \
    third_party/utils/chatgptutil.h \
    third_party/utils/conditionutil.h \
    third_party/utils/emailutil.h \
    third_party/utils/httpuploader.h \
    third_party/utils/microsofttts.h \
    third_party/utils/mysettings.h \
    third_party/utils/pixmaputil.h \
    third_party/utils/simplecalculatorutil.h \
    third_party/utils/string_distance_util.h \
    third_party/utils/tx_nlp.h \
    third_party/utils/warmwishutil.h \
    widgets/buy_vip/buyvipdialog.h \
    widgets/clickablelabel.h \
    widgets/clickablewidget.h \
    widgets/csvviewer.h \
    widgets/custompaintwidget.h \
    widgets/db_browser/dbbrowser.h \
    widgets/db_browser/sqleditor.h \
    widgets/eternal_block_dialog/externalblockdialog.h \
    widgets/eternal_block_dialog/externalblockuser.h \
    widgets/guard_online/guardonlinedialog.h \
    widgets/lucky_draw/luckydrawwindow.h \
    mainwindow/mainwindow.h \
    order_player/clickslider.h \
    order_player/desktoplyricwidget.h \
    order_player/itemselectionlistview.h \
    order_player/logindialog.h \
    order_player/lyricstreamwidget.h \
    order_player/numberanimation.h \
    order_player/orderplayerwindow.h \
    order_player/roundedpixmaplabel.h \
    order_player/songbeans.h \
    third_party/picture_browser/ASCII_Art.h \
    third_party/picture_browser/picturebrowser.h \
    third_party/picture_browser/resizablepicture.h \
    third_party/qrencode/bitstream.h \
    third_party/qrencode/config.h \
    third_party/qrencode/mask.h \
    third_party/qrencode/mmask.h \
    third_party/qrencode/mqrspec.h \
    third_party/qrencode/qrencode.h \
    third_party/qrencode/qrencode_inner.h \
    third_party/qrencode/qrinput.h \
    third_party/qrencode/qrspec.h \
    third_party/qrencode/rsecc.h \
    third_party/qrencode/split.h \
    third_party/utils/myjson.h \
    third_party/utils/xfytts.h \
    widgets/partimagewidget.h \
    widgets/singleentrance.h \
    widgets/smooth_scroll/smoothlistwidget.h \
    widgets/smooth_scroll/smoothscrollbean.h \
    widgets/smooth_scroll/waterfallscrollarea.h \
    widgets/variantviewer.h \
    widgets/video_player/videosurface.h \
    widgets/RoundedAnimationLabel.h \
    widgets/catch_you_dialog/catchyouwidget.h \
    widgets/editor/conditioneditor.h \
    widgets/escape_dialog/escapedialog.h \
    widgets/escape_dialog/hoverbutton.h \
    widgets/eternal_block_dialog/eternalblockdialog.h \
    mainwindow/list_items/eventwidget.h \
    widgets/fluentbutton.h \
    widgets/mytabwidget.h \
    widgets/netinterface.h \
    widgets/login_dialog/qrcodelogindialog.h \
    mainwindow/list_items/replywidget.h \
    widgets/room_status_dialog/roomstatusdialog.h \
    mainwindow/list_items/taskwidget.h \
    third_party/utils/dlog.h \
    third_party/utils/fileutil.h \
    third_party/utils/netutil.h \
    third_party/utils/pinyinutil.h \
    third_party/utils/stringutil.h \
    third_party/utils/textinputdialog.h \
    widgets/video_player/livevideoplayer.h \
    widgets/video_lyric_creator/videolyricscreator.h \
    widgets/windowshwnd.h

FORMS += \
    order_player/importsongsdialog.ui \
    third_party/qss_editor/qsseditdialog.ui \
    widgets/buy_vip/buyvipdialog.ui \
    widgets/db_browser/dbbrowser.ui \
    widgets/guard_online/guardonlinedialog.ui \
    widgets/lucky_draw/luckydrawwindow.ui \
    mainwindow/mainwindow.ui \
    order_player/logindialog.ui \
    order_player/orderplayerwindow.ui \
    third_party/picture_browser/picturebrowser.ui \
    third_party/utils/textinputdialog.ui \
    widgets/singleentrance.ui \
    widgets/video_player/livevideoplayer.ui \
    widgets/catch_you_dialog/catchyouwidget.ui \
    widgets/eternal_block_dialog/eternalblockdialog.ui \
    widgets/login_dialog/qrcodelogindialog.ui \
    widgets/room_status_dialog/roomstatusdialog.ui \
    widgets/video_lyric_creator/videolyricscreator.ui

contains(DEFINES, ENABLE_HTTP_SERVER) {
HEADERS += \
    third_party/http-parser/http_parser.h \
    third_party/qhttpserver/qhttpconnection.h \
    third_party/qhttpserver/qhttprequest.h \
    third_party/qhttpserver/qhttpresponse.h \
    third_party/qhttpserver/qhttpserver.h \
    third_party/qhttpserver/qhttpserverapi.h \
    third_party/qhttpserver/qhttpserverfwd.h

    win32 {
        # win版可以用lib，但是
        LIBS += -L$$PWD/third_party/libs/ -lqhttpserver
    } else {
        SOURCES += \
            third_party/http-parser/http_parser.c \
            third_party/qhttpserver/qhttpconnection.cpp \
            third_party/qhttpserver/qhttprequest.cpp \
            third_party/qhttpserver/qhttpresponse.cpp \
            third_party/qhttpserver/qhttpserver.cpp
    }

INCLUDEPATH += third_party/qhttpserver/ \
    third_party/http-parser/
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES += \
    CHANGELOG.md \
    android/AndroidManifest.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat \
    android/res/values/libs.xml \
    resources/resource.rc \
    Examples.md \
    README.md \
    resources/icons/appicon.ico \
    pictures/神奇弹幕-用法.pdf \
    resource.rc \
    resources/LAVFilters-0.74.1-Installer.exe \
    resources/点歌姬-用法.pdf \
    resources/神奇弹幕-用法.pdf

RESOURCES += \
    resources/resource.qrc

contains(ANDROID_TARGET_ARCH,) {
    ANDROID_PACKAGE_SOURCE_DIR = \
        $$PWD/android
}

android: include(third_party/android_openssl/openssl.pri)

#contains(DEFINES, ENABLE_HTTP_SERVER) {
#    LIBS += -L$$PWD/third_party/libs/ -lqhttpserver
#}

win32: LIBS += -lversion

DEPENDPATH += $$PWD/third_party/libs

contains(ANDROID_TARGET_ARCH,x86) {
    ANDROID_PACKAGE_SOURCE_DIR = \
        $$PWD/android
}

# 使用qBreakpad生成dump
# include($$PWD/third_party/qBreakpad/qBreakpad.pri)
# QMAKE_LIBDIR += $$PWD/third_party/qBreakpad/handler
# LIBS += -lqBreakpad

# 生成pdb文件
# QMAKE_LFLAGS_RELEASE += /MAP
# QMAKE_CFLAGS_RELEASE += /Zi
# QMAKE_LFLAGS_RELEASE += /debug /opt:ref

# 管理员权限
# QMAKE_LFLAGS += /MANIFESTUAC:\"level=\'requireAdministrator\' uiAccess=\'false\'\"
