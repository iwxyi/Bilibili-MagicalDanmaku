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

    // 开启服务器
    qDebug() << "开启总服务器" << port;
    if (!server->listen(static_cast<quint16>(port)))
    {
        ui->serverCheck->setChecked(false);
        statusLabel->setText("开启服务端失败！");
    }

    // 弹幕socket
    danmakuSocketServer = new QWebSocketServer("Danmaku", QWebSocketServer::NonSecureMode, this);
    if (danmakuSocketServer->listen(QHostAddress::Any, quint16(serverPort + DANMAKU_SERVER_PORT)))
    {
        qDebug() << "开启弹幕服务" << serverPort + DANMAKU_SERVER_PORT;
        connect(danmakuSocketServer, &QWebSocketServer::newConnection, this, [=]{
            QWebSocket* clientSocket = danmakuSocketServer->nextPendingConnection();
    //        qDebug() << "danmaku socket 接入" << clientSocket->peerName() << clientSocket->peerAddress() << clientSocket->peerPort();
            danmakuSockets.append(clientSocket);

            connect(clientSocket, &QWebSocket::connected, this, [=]{
                // 一直都是连接状态，不会触发
            });
            connect(clientSocket, &QWebSocket::binaryMessageReceived, this, [=](const QByteArray &message){
                qDebug() << "danmaku message received:" << message;
            });
            connect(clientSocket, &QWebSocket::textMessageReceived, this, [=](const QString &message){
                qDebug() << "danmaku text message received:" << message;
                QJsonParseError error;
                QJsonDocument document = QJsonDocument::fromJson(message.toUtf8(), &error);
                if (error.error != QJsonParseError::NoError)
                {
                    qDebug() << error.errorString() << message;
                    return ;
                }
                QJsonObject json = document.object();
                QString cmd = json.value("cmd").toString();
                if (cmd == "cmds")
                {
                    QStringList sl;
                    QJsonArray arr = json.value("data").toArray();
                    foreach (QJsonValue val, arr)
                        sl << val.toString();
                    danmakuCmdsMaps[clientSocket] = sl;
                }
            });
            connect(clientSocket, &QWebSocket::disconnected, this, [=]{
                danmakuSockets.removeOne(clientSocket);
                if (danmakuCmdsMaps.contains(clientSocket))
                    danmakuCmdsMaps.remove(clientSocket);
                clientSocket->deleteLater();
    //            qDebug() << "danmaku socket 关闭" << danmakuSockets.size();
            });
        });
    }
    else
    {
        qWarning() << "弹幕服务开启失败，端口：" << quint16(serverPort + DANMAKU_SERVER_PORT);
    }


    // 点歌姬socket
    if (musicWindow && !musicSocketServer)
        initMusicServer();
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

    danmakuSocketServer->close();
    danmakuSocketServer->deleteLater();
    danmakuSocketServer = nullptr;
    foreach (QWebSocket* socket, danmakuSockets) {
        socket->close();
        socket->deleteLater();
    }
    danmakuSockets.clear();

    if (musicSocketServer)
    {
        musicSocketServer->close();
        musicSocketServer->deleteLater();
        musicSocketServer = nullptr;

        foreach (QWebSocket* socket, musicSockets) {
            socket->close();
            socket->deleteLater();
        }
        musicSockets.clear();
    }
}

void MainWindow::sendSocketCmd(QString cmd, LiveDanmaku danmaku)
{
    if (!danmakuSocketServer || !danmakuSockets.size()) // 不需要发送，空着的
        return ;

    QJsonObject json;
    json.insert("data", danmaku.toJson());
    json.insert("cmd", cmd);
    QByteArray ba = QJsonDocument(json).toJson();

    foreach (QWebSocket* socket, danmakuSockets)
    {
        if (danmakuCmdsMaps.contains(socket) && !danmakuCmdsMaps[socket].contains(cmd))
            continue;
       socket->sendTextMessage(ba);
    }
}

void MainWindow::initMusicServer()
{
    if (musicSocketServer)
        return ;

    musicSocketServer = new QWebSocketServer("OrderSong", QWebSocketServer::NonSecureMode, this);
    if (!musicSocketServer->listen(QHostAddress::LocalHost, quint16(serverPort + MUSIC_SERVER_PORT)))
    {
        qWarning() << "点歌姬服务开启失败，端口：" << quint16(serverPort + MUSIC_SERVER_PORT);
        delete musicSocketServer;
        musicSocketServer = nullptr;
        return ;
    }
    qDebug() << "开启点歌姬服务" << quint16(serverPort + MUSIC_SERVER_PORT);

    connect(musicSocketServer, &QWebSocketServer::newConnection, this, [=]{
        QWebSocket* clientSocket = musicSocketServer->nextPendingConnection();
//        qDebug() << "music socket 接入" << clientSocket->peerName() << clientSocket->peerAddress() << clientSocket->peerPort();
        musicSockets.append(clientSocket);

        sendMusicList(musicWindow->getOrderSongs(), clientSocket);

        connect(clientSocket, &QWebSocket::connected, this, [=]{
            // 一直都是连接状态，不会触发
        });
        connect(clientSocket, &QWebSocket::binaryMessageReceived, this, [=](const QByteArray &message){
            qDebug() << "music message received:" << message;
        });
        connect(clientSocket, &QWebSocket::textMessageReceived, this, [=](const QString &message){
            qDebug() << "music text message received:" << message;
        });
        connect(clientSocket, &QWebSocket::disconnected, this, [=]{
            musicSockets.removeOne(clientSocket);
            clientSocket->deleteLater();
//            qDebug() << "music socket 关闭" << musicSockets.size();
        });
    });
}

void MainWindow::sendMusicList(const SongList& songs, QWebSocket *socket)
{
    if (!socket && !musicSockets.size()) // 不需要发送，空着的
        return ;

    QJsonObject json;
    QJsonArray array;
    foreach (Song song, songs)
        array.append(song.toJson());
    json.insert("data", array);
    json.insert("cmd", "SONG_LIST");
    QByteArray ba = QJsonDocument(json).toJson();

    if (socket)
    {
        socket->sendTextMessage(ba);
    }
    else
    {
        foreach (QWebSocket* socket, musicSockets)
        {
           socket->sendTextMessage(ba);
        }
    }
}

void MainWindow::syncMagicalRooms()
{
    QString appVersion = GetFileVertion(QApplication::applicationFilePath()).trimmed();
    if (appVersion.startsWith("v") || appVersion.startsWith("V"))
        appVersion.replace(0, 1, "");

    get("http://iwxyi.com/blmagicaldanmaku/enable_room.php?room_id="
        + roomId + "&user_id=" + cookieUid + "&username=" + cookieUname.toUtf8().toPercentEncoding()
        + "&up_uid=" + upUid + "&up_name=" + upName.toUtf8().toPercentEncoding()
        + "&title=" + roomTitle.toUtf8().toPercentEncoding() + "&version=" + appVersion, [=](QJsonObject json){
        // 检测数组
        QJsonArray roomArray = json.value("rooms").toArray();
        magicalRooms.clear();
        foreach (QJsonValue val, roomArray)
        {
            magicalRooms.append(val.toString());
        }

        // 检测新版
        QString lastestVersion = json.value("lastest_version").toString();
        if (lastestVersion.startsWith("v") || lastestVersion.startsWith("V"))
            lastestVersion.replace(0, 1, "");

        if (lastestVersion > appVersion)
        {
            this->appNewVersion = lastestVersion;
            this->appDownloadUrl = json.value("download_url").toString();
            ui->actionUpdate_New_Version->setText("有新版本：" + appNewVersion);
            ui->actionUpdate_New_Version->setIcon(QIcon(":/icons/new_version"));
            ui->actionUpdate_New_Version->setEnabled(true);
            statusLabel->setText("有新版本：" + appNewVersion);
            qDebug() << "有新版本" << appNewVersion << appDownloadUrl;
        }

        QString msg = json.value("message").toString();
        if (!msg.isEmpty())
        {
            QMessageBox::information(this, "神奇弹幕", "msg");
        }

        if(json.value("auto_open").toBool())
        {
            QMessageBox::information(this, "版本更新", "您的版本已过旧，可能存在潜在问题，请尽快更新");
            QDesktopServices::openUrl(appDownloadUrl);
        }

        if (json.value("force_update").toBool())
            QApplication::quit();
    });
}

void MainWindow::serverHandle(QHttpRequest *req, QHttpResponse *resp)
{
    QString urlPath = req->path(); // 示例：/user/abc
    if (urlPath.startsWith("/"))
        urlPath = urlPath.right(urlPath.length() - 1);
    if (urlPath.endsWith("/"))
        urlPath = urlPath.left(urlPath.length() - 1);
    urlPath = urlPath.trimmed();
//    qDebug() << "request ->" << urlPath;
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
    else if (urlPath == "gift") // 礼物专属
    {
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
        else if (isFileType("png|jpg|jpeg|bmp")) // 图片文件
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
            file.open(QIODevice::ReadOnly);
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
