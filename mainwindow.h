#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QSettings>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QDebug>
#include <QApplication>
#include <QClipboard>
#include "netutil.h"
#include "livedanmaku.h"
#include "livedanmakuwindow.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void showEvent(QShowEvent* event) override;
    void closeEvent(QCloseEvent* event) override;

signals:
    void signalNewDanmaku(LiveDanmaku danmaku);

public slots:
    void pullLiveDanmaku();


private slots:
    void on_refreshDanmakuIntervalSpin_valueChanged(int arg1);

    void on_refreshDanmakuCheck_stateChanged(int arg1);

    void on_showLiveDanmakuButton_clicked();

    void on_DiangeAutoCopyCheck_stateChanged(int);

    void on_testDanmakuButton_clicked();

private:
    void appendNewLiveDanmaku(QList<LiveDanmaku> roomDanmakus);
    void newLiveDanmakuAdded(LiveDanmaku danmaku);

private:
    Ui::MainWindow *ui;

    QTimer* danmakuTimer;
    QSettings settings;

    QList<LiveDanmaku> roomDanmakus;
    LiveDanmakuWindow* danmakuWindow = nullptr;
    bool diangeAutoCopy = false;
};
#endif // MAINWINDOW_H
