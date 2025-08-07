#include "mainwindow.h"
#include <QApplication>
// #include <QBreakpadHandler.h>
#include "dlog.h"
#include "signaltransfer.h"
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
CodeRunner* cr;
SignalTransfer* st;

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

    // Mac窗口
    qputenv("QT_MAC_WANTS_LAYER", "1");

    // SetUnhandledExceptionFilter((LPTOP_LEVEL_EXCEPTION_FILTER)ApplicationCrashHandler); // 注册异常捕获程序
    // QBreakpadInstance.setDumpPath(QLatin1String("crashes"));

    QFont font(a.font());
    font.setFamily("微软雅黑");
    a.setFont(font);

    rt = new RuntimeInfo;
    us = nullptr;
    st = new SignalTransfer;
    ac = new AccountInfo;
    pl = new PlatformInfo;
    cr = new CodeRunner;

    QString text = "这是一直重复的吗";

    // 方案1
    QByteArray utf8Bytes = text.toUtf8();
    QByteArray encoded1 = QUrl::toPercentEncoding(utf8Bytes);
    qDebug() << "方案1结果:" << encoded1;

    // 方案2
    QUrl url;
    url.setPath(text);
    QString encoded2 = url.toString(QUrl::PrettyDecoded);
    qDebug() << "方案2结果:" << encoded2;

    // 方案3
    QUrlQuery query;
    query.addQueryItem("text", text);
    QString encoded3 = query.toString(QUrl::PrettyDecoded);
    qDebug() << "方案3结果:" << encoded3;

    // 方案4 - 手动编码
    QString encoded4;
    for (int i = 0; i < utf8Bytes.size(); ++i) {
        unsigned char byte = utf8Bytes.at(i);
        if (byte >= 32 && byte <= 126 && byte != '%') {
            encoded4 += QChar(byte);
        } else {
            encoded4 += QString("%%1").arg(byte, 2, 16, QChar('0')).toUpper();
        }
    }
    qDebug() << "方案4结果:" << encoded4;

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
