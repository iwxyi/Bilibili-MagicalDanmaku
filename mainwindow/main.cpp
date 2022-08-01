#include "mainwindow.h"
#include <QApplication>
#include "dlog.h"

#ifdef Q_OS_WIN32
// #include <dbghelp.h>
/**
 * 崩溃前操作
 */
LONG ApplicationCrashHandler(EXCEPTION_POINTERS *pException)
{
    // 创建 Dump 文件
    /* QString str =QDateTime::currentDateTime().toString("crash_yyyyMMdd")+ QTime::currentTime().toString("HHmmsszzz") + ".dmp";
    HANDLE hDumpFile = CreateFile(LPCWSTR(str.utf16()), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if( hDumpFile != INVALID_HANDLE_VALUE){
        //Dump信息
        MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
        dumpInfo.ExceptionPointers = pException;
        dumpInfo.ThreadId = GetCurrentThreadId();
        dumpInfo.ClientPointers = TRUE;
        //写入Dump文件内容
        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile, MiniDumpNormal, &dumpInfo, NULL, NULL);
    }
    // 这里弹出一个错误对话框并退出程序
    EXCEPTION_RECORD* record = pException->ExceptionRecord;
    QString errCode(QString::number(record->ExceptionCode, 16)),
            errAddr(QString::number(qint64(record->ExceptionAddress), 16)),
            errMode;
    QMessageBox::critical(nullptr, "程序崩溃", QString("错误代码：%1\n错误地址：%2").arg(errCode).arg(errAddr), QMessageBox::Ok); */
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
    // SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)ApplicationCrashHandler); // 注册异常捕获程序

    QFont font(a.font());
    font.setFamily("微软雅黑");
    a.setFont(font);

    rt = new RuntimeInfo;
    us = nullptr;
    ac = new AccountInfo;
    pl = new PlatformInfo;

    MainWindow w;
    if (w.getSettings()->value("debug/logFile", false).toBool())
        qInstallMessageHandler(myMsgOutput);
#if defined(ENABLE_TRAY)
    if (w.getSettings()->value("mainwindow/autoShow", true).toBool()
           || !w.getSettings()->value("mainwindow/enableTray", false).toBool())
#endif
        w.show();
    return a.exec();
}
