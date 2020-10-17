#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("LanYiXi");
    QCoreApplication::setOrganizationDomain("iwxyi.com");
    QCoreApplication::setApplicationName("BilibiliLiveDanmaku");

    MainWindow w;
    w.show();
    return a.exec();
}
