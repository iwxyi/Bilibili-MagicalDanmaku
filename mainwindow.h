#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>
#include "netutil.h"
#include "livedanmaku.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

signals:
    void signalNewDanmaku(LiveDanmaku danmaku);

public slots:
    void pullLiveDanmaku();

    void on_refreshDanmakuIntervalSpin_valueChanged(int arg1);

    void on_refreshDanmakuCheck_stateChanged(int arg1);


private:
    void addNewLiveDanmaku(QList<LiveDanmaku> roomDanmakus);

private:
    Ui::MainWindow *ui;

    QTimer* danmakuTimer;
    QSettings settings;

    QList<LiveDanmaku> roomDanmakus;
};
#endif // MAINWINDOW_H
