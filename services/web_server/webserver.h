#ifndef WEBSERVER_H
#define WEBSERVER_H

#include <QObject>
#include <qhttpserver.h>
#include <QDir>
#include <QWebSocketServer>
#include <QHash>

class WebServer : public QObject
{
    Q_OBJECT
public:
    explicit WebServer(QObject *parent = nullptr);

signals:

public slots:

public:
#ifdef ENABLE_HTTP_SERVER
    QHttpServer *server = nullptr;
#endif
    QString serverDomain;
    qint16 serverPort = 0;
    QDir wwwDir;
    QHash<QString, QString> contentTypeMap;
    QWebSocketServer* extensionSocketServer = nullptr;
    QList<QWebSocket*> extensionSockets;
    QHash<QWebSocket*, QStringList> extensionCmdsMaps;

    bool sendSongListToSockets = false;
    bool sendLyricListToSockets = false;
    bool sendCurrentSongToSockets = false;
};

#endif // WEBSERVER_H
