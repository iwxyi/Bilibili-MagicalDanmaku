#include "mainwindow.h"
#include <QApplication>
#include "utils/dlog.h"

#ifdef Q_OS_WIN32
// 崩溃前操作
LONG ApplicationCrashHandler(EXCEPTION_POINTERS *pException)
{    // 在这里添加处理程序崩溃情况的代码
     /*some function()*/
     return EXCEPTION_EXECUTE_HANDLER;
}
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QCoreApplication::setOrganizationName("LanYiXi");
    QCoreApplication::setOrganizationDomain("iwxyi.com");
    QCoreApplication::setApplicationName("神奇弹幕");

    MainWindow w;
    if (w.getSettings()->value("runtime/debugToFile", false).toBool())
        qInstallMessageHandler(myMsgOutput);
#if defined(ENABLE_TRAY)
    if (w.getSettings()->value("mainwindow/autoShow", true).toBool())
#endif
        w.show();
    return a.exec();
}
