#include "mainwindow.h"
#include "ui_mainwindow.h"

void MainWindow::openServer(int port)
{
    if (!port)
        port = ui->serverPortSpin->value();
    if (port < 1 || port > 65535)
        port = 5520;
    serverPort = qint16(port);

    server = new QHttpServer;
    connect(server, SIGNAL(newRequest(QHttpRequest*, QHttpResponse*)),
            this, SLOT(serverHandle(QHttpRequest*, QHttpResponse*)));

    // 设置服务端参数
    initServerData();

    if (musicWindow && !musicServer)
        initMusicServer();

    qDebug() << "开启总服务器" << port;
    if (!server->listen(static_cast<quint16>(port)))
    {
        ui->serverCheck->setChecked(false);
        statusLabel->setText("开启服务端失败！");
    }
}

void MainWindow::initServerData()
{
    static bool first = true;
    if (!first) // 懒加载
        return ;
    first = true;

    // 读取Content-Type
    QFile suffixFile(":/documents/content_type");
    suffixFile.open(QIODevice::ReadOnly);
    QTextStream suffixIn(&suffixFile);
    suffixIn.setCodec("UTF-8");
    QString line = suffixIn.readLine();
    while (!line.isNull())
    {
        QStringList sl = line.split(" ");
        if (sl.size() == 2)
        {
            QString suffix = sl.at(0);
            QString contentType = sl.at(1);
            contentTypeMap.insert(suffix, contentType);
        }
        line = suffixIn.readLine();
    }
}

void MainWindow::closeServer()
{
    qDebug() << "关闭服务端";
    // server->close(); // 这个不是关闭端口的……
    server->deleteLater();
    server = nullptr;

    if (musicServer)
    {
        musicServer->close();
        musicServer->deleteLater();
        musicServer = nullptr;

        foreach (QTcpSocket* socket, musicSockets) {
            socket->close();
            socket->deleteLater();
        }
        musicSockets.clear();
    }
}

void MainWindow::initMusicServer()
{
    if (musicServer)
        return ;

    musicServer = new QTcpServer(this);
    if (!musicServer->listen(QHostAddress::LocalHost, quint16(serverPort + MUSIC_SERVER_PORT)))
    {
        qWarning() << "点歌姬服务开启失败，端口：" << quint16(serverPort + MUSIC_SERVER_PORT);
        delete musicServer;
        musicServer = nullptr;
        return ;
    }
    qDebug() << "开启点歌姬服务" << quint16(serverPort + MUSIC_SERVER_PORT);

    connect(musicServer, &QTcpServer::newConnection, this, [=]{
        QTcpSocket *clientSocket = musicServer->nextPendingConnection();
        qDebug() << "music socket 接入" << clientSocket->peerName()
                 << clientSocket->peerAddress() << clientSocket->peerPort();
        musicSockets.append(clientSocket);

        connect(clientSocket, &QTcpSocket::connected, this, [=]{
            qDebug() << "client connected" << musicSockets.size();
        });
        connect(clientSocket, &QTcpSocket::stateChanged, this, [=](QAbstractSocket::SocketState state){
            qDebug() << "client state" << state;
        });
        connect(clientSocket, &QTcpSocket::readyRead, this, [=](){
            qDebug() << "client readyRead";
            QByteArray buffer;
            //读取缓冲区数据
            buffer = clientSocket->readAll();
            if(!buffer.isEmpty())
            {
                QString command =  QString::fromLocal8Bit(buffer);
                qDebug() << command;
            }

            clientSocket->write("OK");
        });
        connect(clientSocket, &QTcpSocket::disconnected, this, [=]{
            musicSockets.removeOne(clientSocket);
            clientSocket->deleteLater();
            qDebug() << "music socket 关闭" << musicSockets.size();
        });
    });

}

QByteArray MainWindow::getOrderSongsByteArray(const SongList &songs)
{
    QJsonObject json;
    QJsonArray array;
    foreach (Song song, songs)
        array.append(song.toJson());
    json.insert("songs", array);
    return QJsonDocument(json).toJson();
}

void MainWindow::sendMusicList(const SongList& songs)
{
    QByteArray ba = getOrderSongsByteArray(songs);
    foreach (QTcpSocket* socket, musicSockets)
    {
       socket->write(ba);
    }
}

void MainWindow::serverHandle(QHttpRequest *req, QHttpResponse *resp)
{
    QString urlPath = req->path(); // 示例：/user/abc
    if (urlPath.startsWith("/"))
        urlPath = urlPath.right(urlPath.length() - 1);
    if (urlPath.endsWith("/"))
        urlPath = urlPath.left(urlPath.length() - 1);
    urlPath = urlPath.trimmed();
    qDebug() << "request ->" << urlPath;
    serverHandleUrl(urlPath, req, resp);
}

void MainWindow::serverHandleUrl(QString urlPath, QHttpRequest *req, QHttpResponse *resp)
{
    QByteArray doc;

    auto errorResp = [=](QByteArray err, QHttpResponse::StatusCode code = QHttpResponse::STATUS_BAD_REQUEST) -> void {
        resp->setHeader("Content-Length", snum(err.size()));
        resp->writeHead(code);
        resp->write(err);
        resp->end();
    };

    auto errorStr = [=](QString str, QHttpResponse::StatusCode code = QHttpResponse::STATUS_BAD_REQUEST) -> void {
        return errorResp(str.toStdString().data(), code);
    };

    auto toIndex = [=]() -> void {
        return serverHandleUrl(urlPath + "/index.html", req, resp);
    };

    // 判断文件类型
    QRegularExpressionMatch match;
    QString suffix;
    if (urlPath.indexOf(QRegularExpression("\\.(\\w{1,4})$"), 0, &match) > -1)
        suffix = match.captured(1);
    auto isFileType = [=](QString types) -> bool {
        if (suffix.isEmpty())
            return false;
        return types.indexOf(suffix) > -1;
    };

    // 内容类型
    QString contentType = suffix.isEmpty() ? "text/html"
                                           : contentTypeMap.value(suffix, "application/octet-stream");
    if (contentType.startsWith("text/"))
        contentType += ";charset=utf-8";
    resp->setHeader("Content-Type", contentType);

    // 开始特判
    if (urlPath == "danmaku") // 弹幕姬
    {
        if (!danmakuWindow)
            return errorResp("弹幕姬未开启", QHttpResponse::STATUS_SERVICE_UNAVAILABLE);
        return toIndex();
    }
    else if (urlPath == "music") // 点歌姬
    {
        if (!musicWindow)
            return errorResp("点歌姬未开启", QHttpResponse::STATUS_SERVICE_UNAVAILABLE);
        return toIndex();
    }
    else if (urlPath == "favicon.ico")
    {
        QBuffer buffer(&doc);
        buffer.open(QIODevice::WriteOnly);
        QPixmap(":/icons/star").save(&buffer,"PNG");
    }
    else if (urlPath.isEmpty() // 显示默认的
             && !QFileInfo(wwwDir.absoluteFilePath("index.html")).exists())
    {
        doc = "<html><head><title>神奇弹幕</title></head><body><h1>服务开启成功！</h1></body></html>";
    }
    else if (suffix.isEmpty()) // 没有后缀名，也没有特判的
    {
        return toIndex();
    }
    else // 设置文件
    {
        QString filePath = wwwDir.absoluteFilePath(urlPath);
        QFile file(filePath);
        if (!file.exists())
        {
            qWarning() << "文件：" << filePath << "不存在";
            return errorStr("路径：" + urlPath + " 无法访问！", QHttpResponse::STATUS_NOT_FOUND);
        }
        else if (isFileType("png|jpg|jpeg|bmp|gif")) // 图片文件
        {
            QByteArray imageType = "png";
            if (suffix == "gif")
                imageType = "gif";
            else if (suffix == "jpg" || suffix == "jpeg")
                imageType = "jpeg";

            // 图片文件，需要特殊加载
            QBuffer buffer(&doc);
            buffer.open(QIODevice::WriteOnly);
            QPixmap(filePath).save(&buffer, "png"); //  必须要加format，默认的无法显示
        }
        else // 不需要处理或者未知类型的文件
        {
            // html、txt、JS、CSS等，直接读取文件
            file.open(QIODevice::ReadOnly | QIODevice::Text);
            doc = file.readAll();
            file.close();
        }
    }

    // 开始返回
    resp->setHeader("Content-Length", snum(doc.size()));
    resp->writeHead(QHttpResponse::STATUS_OK);
    resp->write(doc);
    resp->end();
}
