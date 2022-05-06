#include "mainwindow.h"
#include <QApplication>
#include "dlog.h"

#ifdef Q_OS_WIN32
// 崩溃前操作
LONG ApplicationCrashHandler(EXCEPTION_POINTERS *pException)
{    // 在这里添加处理程序崩溃情况的代码
     /*some function()*/
     return EXCEPTION_EXECUTE_HANDLER;
}
#endif

RuntimeInfo* rt;
UserSettings* us;
AccountInfo* ac;
PlatformInfo* pl;

int main(int argc, char *argv[])
{
    QApplication::setDesktopSettingsAware(true); // 据说是避免不同分辨率导致显示的字体大小不一致
#if (QT_VERSION > QT_VERSION_CHECK(5,6,0))
    // 设置高分屏属性以便支持2K4K等高分辨率，尤其是手机app。
    // 必须写在main函数的QApplication a(argc, argv);的前面
    // 设置后，读取到的窗口会随着显示器倍数而缩小
    QGuiApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif

    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("LanYiXi");
    QCoreApplication::setOrganizationDomain("iwxyi.com");
    QCoreApplication::setApplicationName("神奇弹幕");

    QFont font(a.font());
    font.setFamily("微软雅黑");
    a.setFont(font);

    rt = new RuntimeInfo;
    us = new UserSettings("data.ini");
    ac = new AccountInfo;
    pl = new PlatformInfo;

    MainWindow w;
    if (w.getSettings()->value("runtime/debugToFile", false).toBool())
        qInstallMessageHandler(myMsgOutput);
#if defined(ENABLE_TRAY)
    if (w.getSettings()->value("mainwindow/autoShow", true).toBool()
           || !w.getSettings()->value("mainwindow/enableTray", false).toBool())
#endif
        w.show();
    return a.exec();
}
