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
#include <QNetworkCookie>
#include <QMessageBox>
#include <QInputDialog>
#include <QTextCodec>
#include <stdio.h>
#include <iostream>
#include "netutil.h"
#include "livedanmaku.h"
#include "livedanmakuwindow.h"
#include "taskwidget.h"

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
    void signalRemoveDanmaku(LiveDanmaku danmaku);

public slots:
    void pullLiveDanmaku();


private slots:
    void on_refreshDanmakuIntervalSpin_valueChanged(int arg1);

    void on_refreshDanmakuCheck_stateChanged(int arg1);

    void on_showLiveDanmakuButton_clicked();

    void on_DiangeAutoCopyCheck_stateChanged(int);

    void on_testDanmakuButton_clicked();

    void on_removeDanmakuIntervalSpin_valueChanged(int arg1);

    void on_roomIdEdit_editingFinished();

    void on_languageAutoTranslateCheck_stateChanged(int arg1);

    void on_tabWidget_tabBarClicked(int index);

    void on_refreshDanmakuCheck_clicked();

    void on_SetBrowserCookieButton_clicked();

    void on_SetBrowserDataButton_clicked();

    void on_SetBrowserHelpButton_clicked();

    void on_SendMsgButton_clicked();

    void on_AIReplyCheck_stateChanged(int arg1);

    void on_testDanmakuEdit_returnPressed();

    void on_SendMsgEdit_returnPressed();

    void on_taskListWidget_customContextMenuRequested(const QPoint &pos);

    void on_addTaskButton_clicked();

private:
    void appendNewLiveDanmaku(QList<LiveDanmaku> roomDanmakus);
    void newLiveDanmakuAdded(LiveDanmaku danmaku);
    void oldLiveDanmakuRemoved(LiveDanmaku danmaku);

    void sendMsg(QString msg);

    void addTimerTask(bool enable, int second, QString text);
    void saveTaskList();
    void restoreTaskList();

private:
    Ui::MainWindow *ui;
    QSettings settings;

    QTimer* danmakuTimer;
    qint64 removeDanmakuInterval = 20000;
    QList<LiveDanmaku> roomDanmakus;
    qint64 prevLastDanmakuTimestamp = 0;
    bool firstPullDanmaku = true; // 是否不加载以前的弹幕
    LiveDanmakuWindow* danmakuWindow = nullptr;
    bool diangeAutoCopy = false;

    QString browserCookie;
    QString browserData;
    QTimer* sendMsgTimer;

    QLabel* statusLabel;
};
#endif // MAINWINDOW_H
