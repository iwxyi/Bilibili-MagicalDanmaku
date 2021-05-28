#include "basewindow.h"
#include "ui_mainwindow.h"
#include "fileutil.h"

void BaseWindow::openServer(int port)
{
    if (!port)
        port = ui->serverPortSpin->value();
    if (port < 1 || port > 65535)
        port = 5520;
    serverPort = qint16(port);
#if defined(ENABLE_HTTP_SERVER)
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

    openSocketServer();
#endif
}

void BaseWindow::openSocketServer()
{
    // 弹幕socket
    danmakuSocketServer = new QWebSocketServer("Danmaku", QWebSocketServer::NonSecureMode, this);
    if (danmakuSocketServer->listen(QHostAddress::Any, quint16(serverPort + DANMAKU_SERVER_PORT)))
    {
        qDebug() << "开启弹幕服务" << serverPort + DANMAKU_SERVER_PORT;
        connect(danmakuSocketServer, &QWebSocketServer::newConnection, this, [=]{
            QWebSocket* clientSocket = danmakuSocketServer->nextPendingConnection();
            qDebug() << "danmaku socket 接入" << clientSocket->peerName() << clientSocket->peerAddress() << clientSocket->peerPort();
            danmakuSockets.append(clientSocket);

            connect(clientSocket, &QWebSocket::connected, this, [=]{
                // 一直都是连接状态，不会触发
            });
            connect(clientSocket, &QWebSocket::binaryMessageReceived, this, [=](const QByteArray &message){
                qDebug() << "danmaku binary message received:" << message;
            });
            connect(clientSocket, &QWebSocket::textMessageReceived, this, [=](const QString &message){
                qDebug() << "danmaku message received:" << message;
                processSocketTextMsg(clientSocket, message);
            });
            connect(clientSocket, &QWebSocket::disconnected, this, [=]{
                danmakuSockets.removeOne(clientSocket);
                if (danmakuCmdsMaps.contains(clientSocket))
                {
                    QStringList cmds = danmakuCmdsMaps[clientSocket];
                    danmakuCmdsMaps.remove(clientSocket);

                    auto setFalseIfNone = [=](bool& enabled, QString key) {
                        if (!enabled || !cmds.contains(key))
                            return ;

                        foreach (QStringList sl, danmakuCmdsMaps) {
                            if (sl.contains(key))
                                return ;
                        }
                        enabled = false;
                    };

                    setFalseIfNone(sendSongListToSockets, "SONG_LIST");
                    setFalseIfNone(sendLyricListToSockets, "LYRIC_LIST");
                    setFalseIfNone(sendCurrentSongToSockets, "CURRENT_SONG");
                }
                clientSocket->deleteLater();
                qDebug() << "danmaku socket 关闭" << danmakuSockets.size();
            });

            triggerCmdEvent("NEW_WEBSOCKET", LiveDanmaku());
        });
    }
    else
    {
        qCritical() << "弹幕服务开启失败，端口：" << quint16(serverPort + DANMAKU_SERVER_PORT);
    }
}

void BaseWindow::processSocketTextMsg(QWebSocket *clientSocket, const QString &message)
{
    QJsonParseError error;
    QJsonDocument document = QJsonDocument::fromJson(message.toUtf8(), &error);
    if (error.error != QJsonParseError::NoError)
    {
        qCritical() << error.errorString() << message;
        return ;
    }
    QJsonObject json = document.object();
    QString cmd = json.value("cmd").toString().toUpper();
    if (cmd == "CMDS") // 最开始的筛选cmd
    {
        QStringList sl;
        QJsonArray arr = json.value("data").toArray();
        foreach (QJsonValue val, arr)
            sl << val.toString();
        danmakuCmdsMaps[clientSocket] = sl;

        // 点歌相关
        if (musicWindow)
        {
            // 点歌列表
            if (sl.contains("SONG_LIST"))
            {
                sendSongListToSockets = true;
                sendMusicList(musicWindow->getOrderSongs(), clientSocket);
            }

            // 歌词
            if (sl.contains("LYRIC_LIST"))
            {
                sendLyricListToSockets = true;
                sendLyricList(clientSocket);
            }

            // 当前歌曲变更
            if (sl.contains("CURRENT_SONG"))
            {
                sendCurrentSongToSockets = true;
                sendJsonToSockets("CURRENT_SONG", musicWindow->getPlayingSong().toJson(), clientSocket);
            }
        }

        // 本场直播所有礼物
        if (sl.contains("LIVE_ALL_GIFTS"))
        {
            // 这里不排序，直接发送
            QJsonObject json;
            json.insert("guards", LiveDanmaku::toJsonArray(liveAllGuards));
            json.insert("gifts", LiveDanmaku::toJsonArray(liveAllGifts));
            sendJsonToSockets("LIVE_ALL_GIFTS", json, clientSocket);
        }

        triggerCmdEvent("WEBSOCKET_CMDS", LiveDanmaku(sl.join(",")));
    }
    else if (!ui->allowWebControlCheck->isChecked())
    {
        qWarning() << "无效的CMD，或者未开启网页控制：" << cmd;
        return ;
    }
    else if (cmd == "FORWARD") // 转发给其他socket
    {
        QJsonObject data = json.value("data").toObject();
        QString cmd2 = data.value("cmd").toString();
        sendTextToSockets(cmd2, QJsonDocument(data).toJson());
    }
    else if (cmd == "SET_VALUE") // 修改本地配置
    {
        QJsonObject data = json.value("data").toObject();
        QString key = data.value("key").toString();
        QJsonValue val = data.value("value");
        if (val.isString())
            settings->setValue(key, val.toString());
        else if (val.isBool())
            settings->setValue(key, val.toBool());
        else if (val.isDouble())
            settings->setValue(key, val.toDouble());
        else
            settings->setValue(key, val.toVariant());
    }
    else if (cmd == "SEND_MSG") // 直接发送弹幕（允许多行，但不含变量）
    {
        QString text = json.value("data").toString();
        qDebug() << "发送远程弹幕：" << text;
        sendAutoMsg(text);
    }
    else if (cmd == "SEND_VARIANT_MSG") // 发送带有变量的弹幕
    {
        QString text = json.value("data").toString();
        text = processDanmakuVariants(text, LiveDanmaku());
        qDebug() << "发送远程弹幕或命令：" << text;
        sendAutoMsg(text);
    }
}

void BaseWindow::initServerData()
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

    // 创建缓存文件夹
    ensureDirExist(webCache(""));
}

void BaseWindow::closeServer()
{
#if defined(ENABLE_HTTP_SERVER)
    qDebug() << "关闭服务端";
    // server->close(); // 这个不是关闭端口的……
    server->deleteLater();
    server = nullptr;
#endif

    danmakuSocketServer->close();
    danmakuSocketServer->deleteLater();
    danmakuSocketServer = nullptr;
    foreach (QWebSocket* socket, danmakuSockets) {
        socket->close();
        socket->deleteLater();
    }
    danmakuSockets.clear();
}

void BaseWindow::sendDanmakuToSockets(QString cmd, LiveDanmaku danmaku)
{
    if (!danmakuSocketServer || !danmakuSockets.size()) // 不需要发送，空着的
        return ;

    QJsonObject json;
    json.insert("cmd", cmd);
    json.insert("data", danmaku.toJson());
    QByteArray ba = QJsonDocument(json).toJson();

    foreach (QWebSocket* socket, danmakuSockets)
    {
        if (!danmakuCmdsMaps.contains(socket) || !danmakuCmdsMaps[socket].contains(cmd))
            continue;
       socket->sendTextMessage(ba);
    }
}

void BaseWindow::sendJsonToSockets(QString cmd, QJsonValue data, QWebSocket *socket)
{
    if (!socket && !danmakuSockets.size()) // 不需要发送，空着的
        return ;

    QJsonObject json;
    json.insert("cmd", cmd);
    json.insert("data", data);
    QByteArray ba = QJsonDocument(json).toJson();
    sendTextToSockets(cmd, ba, socket);
}

void BaseWindow::sendTextToSockets(QString cmd, QByteArray data, QWebSocket *socket)
{
    if (!socket && !danmakuSockets.size())
        return ;

    if (socket)
    {
        socket->sendTextMessage(data);
    }
    else
    {
        SOCKET_DEB << "发送至每个socket" << cmd << data;
        foreach (QWebSocket* socket, danmakuSockets)
        {
            if (danmakuCmdsMaps.contains(socket) && danmakuCmdsMaps[socket].contains(cmd))
            {
                socket->sendTextMessage(data);
            }
        }
    }
}

void BaseWindow::sendMusicList(const SongList& songs, QWebSocket *socket)
{
    if (!sendSongListToSockets || (!socket && !danmakuSockets.size()) || !musicWindow) // 不需要发送，空着的
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
        foreach (QWebSocket* socket, danmakuSockets)
        {
            if (!danmakuCmdsMaps.contains(socket) || danmakuCmdsMaps[socket].contains("SONG_LIST"))
                socket->sendTextMessage(ba);
        }
    }
}

void BaseWindow::sendLyricList(QWebSocket *socket)
{
    if (!sendLyricListToSockets || (!socket && !danmakuSockets.size()) || !musicWindow)
        return ;

    QJsonObject json;
    QJsonArray array;
    QStringList lyrics = musicWindow->getSongLyrics(ui->songLyricsToFileMaxSpin->value());
    json.insert("data", lyrics.join("\n"));
    json.insert("cmd", "LYRIC_LIST");
    QByteArray ba = QJsonDocument(json).toJson();

    if (socket)
    {
        socket->sendTextMessage(ba);
    }
    else
    {
        foreach (QWebSocket* socket, danmakuSockets)
        {
            if (!danmakuCmdsMaps.contains(socket) || danmakuCmdsMaps[socket].contains("LYRIC_LIST"))
                socket->sendTextMessage(ba);
        }
    }
}

QString BaseWindow::webCache(QString name) const
{
    return dataPath + "cache/" + name;
}

void BaseWindow::syncMagicalRooms()
{
    QString appVersion = GetFileVertion(QApplication::applicationFilePath()).trimmed();
    if (appVersion.startsWith("v") || appVersion.startsWith("V"))
        appVersion.replace(0, 1, "");

    get("https://iwxyi.com/blmagicaldanmaku/enable_room.php?room_id="
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

        QString msg = json.value("msg").toString();
        if (!msg.isEmpty())
        {
            QMessageBox::information(this, "神奇弹幕", msg);
        }

        if(json.value("auto_open").toBool())
        {
            QMessageBox::information(this, "版本更新", "您的版本已过旧，可能存在潜在问题，请尽快更新\n" + appVersion + " => " + appNewVersion);
            QDesktopServices::openUrl(appDownloadUrl);
        }

        if (json.value("force_update").toBool())
            QApplication::quit();
    });
}

#if defined(ENABLE_HTTP_SERVER)
void BaseWindow::serverHandle(QHttpRequest *req, QHttpResponse *resp)
{
    // 解析参数
    QString fullUrl = req->url().toString(); // 包含：/user/header?uid=123
    QHash<QString, QString> params;
    QString urlPath = fullUrl;
    if (fullUrl.contains("?"))
    {
        int pos = fullUrl.indexOf("?");
        urlPath = fullUrl.left(pos);
        QStringList sl = fullUrl.right(fullUrl.length() - pos - 1).split("&"); // a=1&b=2的参数
        foreach (auto s, sl)
        {
            int pos = s.indexOf("=");
            if (pos == -1)
            {
                params.insert(QByteArray::fromPercentEncoding(s.toUtf8()), "");
                continue;
            }

            QString key = s.left(pos);
            QString val = s.right(s.length() - pos - 1);
            params.insert(QByteArray::fromPercentEncoding(key.toUtf8()),
                          QByteArray::fromPercentEncoding(val.toUtf8()));
        }
    }

    // 路径
    // QString urlPath = req->path(); // 示例：/user/abc，不包括 域名/
    if (urlPath.startsWith("/"))
        urlPath = urlPath.right(urlPath.length() - 1);
    if (urlPath.endsWith("/"))
        urlPath = urlPath.left(urlPath.length() - 1);
    urlPath = urlPath.trimmed();

//    qDebug() << "request ->" << urlPath;
    serverHandleUrl(urlPath, params, req, resp);
}

void BaseWindow::serverHandleUrl(const QString &urlPath, QHash<QString, QString> &params, QHttpRequest *req, QHttpResponse *resp)
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


    auto toIndex = [&]() -> void {
        return serverHandleUrl(urlPath + "/index.html", params, req, resp);
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

    // 开始特判
    // ========== 各类接口 ==========
    if (urlPath.startsWith("api/"))
    {
        doc = getApiContent(urlPath.right(urlPath.length() - 4), params, &contentType);
    }
    // ========== 固定类型 ==========
    else if (urlPath == "danmaku") // 弹幕姬
    {
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
    else if (urlPath.isEmpty() // 显示默认的
             && !QFileInfo(wwwDir.absoluteFilePath("index.html")).exists())
    {
        doc = "<html><head><title>神奇弹幕</title></head><body><h1>服务开启成功！</h1></body></html>";
    }
    else if (suffix.isEmpty() && !isFileExist(wwwDir.absoluteFilePath(urlPath))) // 没有后缀名，也没有特判的
    {
        return toIndex();
    }
    // ========== 特殊文件 ==========
    else if (urlPath == "favicon.ico")
    {
        QBuffer buffer(&doc);
        buffer.open(QIODevice::WriteOnly);
        QPixmap(":/icons/star").save(&buffer,"PNG");
    }
    // ========== 普通文件 ==========
    else if (urlPath == "favicon.ico")
    {
        QBuffer buffer(&doc);
        buffer.open(QIODevice::WriteOnly);
        QPixmap(":/icons/star").save(&buffer,"PNG");
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
            if (contentType.startsWith("text/") || contentType.endsWith("script"))
                processServerVariant(doc);
            file.close();
        }
    }

    // 开始返回
    if (contentType.startsWith("text/"))
        contentType += ";charset=utf-8";
    resp->setHeader("Content-Type", contentType);
    resp->setHeader("Content-Length", snum(doc.size()));
    resp->setHeader("Access-Control-Allow-Origin", "*");
    resp->writeHead(QHttpResponse::STATUS_OK);
    resp->write(doc);
    resp->end();
}
#endif

void BaseWindow::processServerVariant(QByteArray &doc)
{
    doc.replace("__DOMAIN__", serverDomain.toUtf8())
            .replace("__PORT__", snum(serverPort).toUtf8())
            .replace("__WS_PORT__", snum(serverPort+1).toUtf8());
}

/// 一些header相关的
/// @param url 不包括前缀api/，直达动作本身
/// 示例地址：http://__DOMAIN__:__PORT__/api/header?uid=123456
QByteArray BaseWindow::getApiContent(QString url, QHash<QString, QString> params, QString* contentType)
{
    QByteArray ba;
    if (url == "header")
    {
        *contentType = "image/jpeg";
        if (!params.contains("uid"))
            return ba;

        QString uid = params.value("uid");
        QString filePath = webCache("header_" + uid);
        if (!isFileExist(filePath))
        {
            // 获取封面URL并下载封面
            MyJson json(NetUtil::getWebData("http://api.bilibili.com/x/space/acc/info?mid=" + uid));
            NetUtil::downloadWebFile(json.data().s("face"), filePath);
        }

        // 返回文件
        QFile f(filePath);
        f.open(QIODevice::ReadOnly);
        ba = f.readAll();
        f.close();
    }

    return ba;
}
