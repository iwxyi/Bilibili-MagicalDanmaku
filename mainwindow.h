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
#include <QtWebSockets/QWebSocket>
#include <QAuthenticator>
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

    struct HostInfo
    {
        QString host;
        int port;
        int wss_port;
        int ws_port;
    };

    struct HeaderStruct
    {
        int totalSize;
        short headerSize;
        short ver;
        int operation;
        int seqId;
    };

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

    void on_languageAutoTranslateCheck_stateChanged(int);

    void on_tabWidget_tabBarClicked(int index);

    void on_refreshDanmakuCheck_clicked();

    void on_SetBrowserCookieButton_clicked();

    void on_SetBrowserDataButton_clicked();

    void on_SetBrowserHelpButton_clicked();

    void on_SendMsgButton_clicked();

    void on_AIReplyCheck_stateChanged(int);

    void on_testDanmakuEdit_returnPressed();

    void on_SendMsgEdit_returnPressed();

    void on_taskListWidget_customContextMenuRequested(const QPoint &);

    void on_addTaskButton_clicked();

    void sendMsg(QString msg);

    void slotSocketError(QAbstractSocket::SocketError error);

private:
    void appendNewLiveDanmakus(QList<LiveDanmaku> roomDanmakus);
    void newLiveDanmakuAdded(LiveDanmaku danmaku);
    void oldLiveDanmakuRemoved(LiveDanmaku danmaku);

    void addTimerTask(bool enable, int second, QString text);
    void saveTaskList();
    void restoreTaskList();

    QVariant getCookies();
    void initWS();
    void startConnectWS();
    void getRoomInit();
    void getRoomInfo();
    void getDanmuInfo();
    void startMsgLoop();
    QByteArray makePack(QByteArray body, qint32 operation);
    void sendVeriPacket();
    void sendHeartPacket();

private:
    Ui::MainWindow *ui;
    QSettings settings;
    QString roomId;

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

    QString shortId;
    QString uid;
    QList<HostInfo> hostList;
    QString token;

    QWebSocket* socket;
    QTimer* heartTimer;
};
#endif // MAINWINDOW_H
