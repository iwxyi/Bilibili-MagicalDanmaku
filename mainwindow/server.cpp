#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "fileutil.h"
#include "stringutil.h"

void MainWindow::openServer(int port)
{
    if (!port)
        port = ui->serverPortSpin->value();
    if (port < 1 || port > 65535)
        port = 5520;
#if defined(ENABLE_HTTP_SERVER)
    if (!webServer->server)
    {
        webServer->server = new QHttpServer;
        connect(webServer->server, SIGNAL(newRequest(QHttpRequest*, QHttpResponse*)),
                this, SLOT(serverHandle(QHttpRequest*, QHttpResponse*)));

        // 设置服务端参数
        initServerData();
    }
    else
    {
        // 判断需不需要重新开启
        if (webServer->serverPort == qint16(port))
        {
            qInfo() << "Web服务端口一致，不需要重启";
            return;
        }

        qDebug() << "重新开启 HTTP 服务";
        closeServer(); // webServer->server=null，重新进入上面if的初始化方法，之后再打开
        openServer(port);
        return;
    }

    // 开启服务器
    qInfo() << "开启 HTTP 服务" << port;
    webServer->serverPort = qint16(port);
    if (!webServer->server->listen(static_cast<quint16>(port)))
    {
        qWarning() << "开启 HTTP 服务失败！";
        ui->serverCheck->setChecked(false);
        statusLabel->setText("开启服务端失败！");
    }

    openSocketServer();
#endif
}

void MainWindow::openSocketServer()
{
    auto updateConnectCount = [=]{
        ui->existExtensionsLabel->setToolTip("当前已连接的页面数：" + snum(webServer->extensionSockets.size()));
    };

    // 弹幕socket
    qInfo() << "开启 Socket 服务" << webServer->serverPort + DANMAKU_SERVER_PORT;
    webServer->extensionSocketServer = new QWebSocketServer("Danmaku", QWebSocketServer::NonSecureMode, this);
    if (webServer->extensionSocketServer->listen(QHostAddress::Any, quint16(webServer->serverPort + DANMAKU_SERVER_PORT)))
    {
        connect(webServer->extensionSocketServer, &QWebSocketServer::newConnection, this, [=]{
            QWebSocket* clientSocket = webServer->extensionSocketServer->nextPendingConnection();
            qInfo() << "ext socket 接入" << webServer->extensionSockets.size() << clientSocket->peerAddress() << clientSocket->peerPort() << clientSocket->peerName();
            webServer->extensionSockets.append(clientSocket);
            updateConnectCount();

            connect(clientSocket, &QWebSocket::connected, this, [=]{
                // 一直都是连接状态，不会触发
            });
            connect(clientSocket, &QWebSocket::binaryMessageReceived, this, [=](const QByteArray &message){
                qInfo() << "ext binary message received:" << message;
            });
            connect(clientSocket, &QWebSocket::textMessageReceived, this, [=](const QString &message){
                qInfo() << "ext message received:" << message;
                processSocketTextMsg(clientSocket, message);
            });
            connect(clientSocket, &QWebSocket::disconnected, this, [=]{
                webServer->extensionSockets.removeOne(clientSocket);
                if (webServer->extensionCmdsMaps.contains(clientSocket))
                {
                    QStringList cmds = webServer->extensionCmdsMaps[clientSocket];
                    webServer->extensionCmdsMaps.remove(clientSocket);

                    auto setFalseIfNone = [=](bool& enabled, QString key) {
                        if (!enabled || !cmds.contains(key))
                            return ;

                        foreach (QStringList sl, webServer->extensionCmdsMaps) {
                            if (sl.contains(key))
                                return ;
                        }
                        enabled = false;
                    };

                    setFalseIfNone(webServer->sendSongListToSockets, "SONG_LIST");
                    setFalseIfNone(webServer->sendLyricListToSockets, "LYRIC_LIST");
                    setFalseIfNone(webServer->sendCurrentSongToSockets, "CURRENT_SONG");
                }
                clientSocket->deleteLater();
                updateConnectCount();
                qInfo() << "ext socket 关闭" << webServer->extensionSockets.size();
            });

            triggerCmdEvent("NEW_WEBSOCKET", LiveDanmaku());
        });
    }
    else
    {
        qCritical() << "弹幕服务开启失败，端口：" << quint16(webServer->serverPort + DANMAKU_SERVER_PORT);
    }
}

void MainWindow::processSocketTextMsg(QWebSocket *clientSocket, const QString &message)
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
    if (!cmd.isEmpty())
    {
        triggerCmdEvent("SOCKET:" + cmd, LiveDanmaku().with(json));
    }
    if (cmd == "CMDS") // 最开始的筛选cmd
    {
        QStringList sl;
        QJsonArray arr = json.value("data").toArray();
        foreach (QJsonValue val, arr)
            sl << val.toString();
        webServer->extensionCmdsMaps[clientSocket] = sl;

        // 点歌相关
        if (musicWindow)
        {
            // 点歌列表
            if (sl.contains("SONG_LIST"))
            {
                webServer->sendSongListToSockets = true;
                sendMusicList(musicWindow->getOrderSongs(), clientSocket);
            }

            // 歌词
            if (sl.contains("LYRIC_LIST"))
            {
                webServer->sendLyricListToSockets = true;
                sendLyricList(clientSocket);
            }

            // 当前歌曲变更
            if (sl.contains("CURRENT_SONG"))
            {
                webServer->sendCurrentSongToSockets = true;
                sendJsonToSockets("CURRENT_SONG", musicWindow->getPlayingSong().toJson(), clientSocket);
            }
        }

        // 本场直播所有礼物
        if (sl.contains("LIVE_ALL_GIFTS"))
        {
            // 这里不排序，直接发送
            QJsonObject json;
            json.insert("guards", LiveDanmaku::toJsonArray(liveService->liveAllGuards));
            json.insert("gifts", LiveDanmaku::toJsonArray(liveService->liveAllGifts));
            sendJsonToSockets("LIVE_ALL_GIFTS", json, clientSocket);
        }

        triggerCmdEvent("WEBSOCKET_CMDS", LiveDanmaku(sl.join(",")));
    }
    else if (cmd == "SET_CONFIG")
    {
        // "data": { "key1":123, "key2": "string" }
        QJsonObject data = json.value("data").toObject();
        QString group = json.value("group").toString();
        if (!group.isEmpty())
            cr->extSettings->beginGroup(group);
        foreach (auto key, data.keys())
        {
            auto v = data.value(key).toVariant();
            // extSettings->setValue(key, v); // 存的是QVariant->string/int/bool...
            cr->extSettings->setValue(key, data.value(key)); // QJsonValue
            qInfo() << "保存配置：" << key << v << cr->extSettings->value(key).toJsonValue();
        }
        if (!group.isEmpty())
            cr->extSettings->endGroup();
    }
    else if (cmd == "GET_CONFIG")
    {
        // "data": ["key1", "key2", ...]
        QJsonArray arr = json.value("data").toArray();
        QJsonObject rst;
        QString group = json.value("group").toString();
        if (!group.isEmpty())
            cr->extSettings->beginGroup(group);
        if (arr.size()) // 返回指定配置
        {
            foreach (QJsonValue val, arr)
            {
                QString key = val.toString();
                // auto v = QJsonValue::fromVariant(extSettings->value(key));
                auto v = cr->extSettings->value(key).toJsonValue(); // QJsonValue
                rst.insert(key, v);
            }
        }
        else // 返回所有配置
        {
            auto keys = cr->extSettings->allKeys();
            foreach (auto key, keys)
            {
                // auto v = QJsonValue::fromVariant(extSettings->value(key)); // QVariant->string
                auto v = cr->extSettings->value(key).toJsonValue(); // QJsonValue
                rst.insert(key, v);
                // qDebug() << "读取配置：" << key << extSettings->value(key).toJsonValue() << extSettings->value(key) << extSettings->value(key).typeName();
            }
        }
        if (!group.isEmpty())
            cr->extSettings->endGroup();
        qInfo() << "返回配置：" << rst;
        sendJsonToSockets("GET_CONFIG", rst, clientSocket);
    }
    else if (cmd == "GET_INFO")
    {
        // "data": { "room_id": "%room_id%", "title": "%room_title%" }
        QJsonObject obj = json.value("data").toObject();
        auto keys = obj.keys();
        QJsonObject lis;
        foreach (auto key, keys)
        {
            QString val = obj.value(key).toString();
            val = cr->processDanmakuVariants(val, LiveDanmaku());
            lis.insert(key, val);
        }
        sendJsonToSockets("GET_INFO", lis, clientSocket);
    }
    else if (cmd == "FORWARD") // 转发给其他socket
    {
        if (!ui->allowWebControlCheck->isChecked()) // 允许网页控制
            return showError("未开启解锁安全限制", "无法执行：" + cmd);
        QJsonObject data = json.value("data").toObject();
        QString cmd2 = data.value("cmd").toString();
        sendTextToSockets(cmd2, QJsonDocument(data).toJson());
    }
    else if (cmd == "SET_VALUE") // 修改本地配置
    {
        if (!ui->allowWebControlCheck->isChecked()) // 允许网页控制
            return showError("未开启解锁安全限制", "无法执行：" + cmd);
        QJsonObject data = json.value("data").toObject();
        if (data.size() == 2 && data.contains("key") && data.contains("value"))
        {
            // 单独设置： data:{}
            QString key = data.value("key").toString();
            QJsonValue val = data.value("value");
            if (val.isString())
                us->setValue(key, val.toString());
            else if (val.isBool())
                us->setValue(key, val.toBool());
            else if (val.isDouble())
                us->setValue(key, val.toDouble());
            else
                us->setValue(key, val.toVariant());
        }
        else
        {
            foreach (auto key, data.keys())
            {
                QJsonValue val = data.value(key);
                if (val.isString())
                    us->setValue(key, val.toString());
                else if (val.isBool())
                    us->setValue(key, val.toBool());
                else if (val.isDouble())
                    us->setValue(key, val.toDouble());
                else
                    us->setValue(key, val.toVariant());
            }
        }
    }
    else if (cmd == "SEND_MSG") // 直接发送弹幕（允许多行，但不含变量）
    {
        if (!ui->allowWebControlCheck->isChecked()) // 允许网页控制
            return showError("未开启解锁安全限制", "无法执行：" + cmd);
        QString text = json.value("data").toString();
        qInfo() << "发送远程弹幕：" << text;
        cr->sendAutoMsg(text, LiveDanmaku());
    }
    else if (cmd == "SEND_VARIANT_MSG") // 发送带有变量的弹幕
    {
        if (!ui->allowWebControlCheck->isChecked()) // 允许网页控制
            return showError("未开启解锁安全限制", "无法执行：" + cmd);
        QString text = json.value("data").toString();
        text = cr->processDanmakuVariants(text, LiveDanmaku());
        qInfo() << "发送远程弹幕或命令：" << text;
        cr->sendAutoMsg(text, LiveDanmaku());
    }
    else
    {
        // 触发收到消息事件
        triggerCmdEvent("SOCKET_MSG_RECEIVED", LiveDanmaku().with(json));
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
            webServer->contentTypeMap.insert(suffix, contentType);
        }
        line = suffixIn.readLine();
    }

    // 创建缓存文件夹
    ensureDirExist(webCache(""));
}

void MainWindow::closeServer()
{
#if defined(ENABLE_HTTP_SERVER)
    qInfo() << "关闭服务端";
    webServer->server->deleteLater();
    webServer->server = nullptr;
#endif

    webServer->extensionSocketServer->close();
    webServer->extensionSocketServer->deleteLater();
    webServer->extensionSocketServer = nullptr;
    foreach (QWebSocket* socket, webServer->extensionSockets) {
        socket->close();
        socket->deleteLater();
    }
    webServer->extensionSockets.clear();
}

void MainWindow::sendDanmakuToSockets(QString cmd, LiveDanmaku danmaku)
{
    if (!webServer->extensionSocketServer || !webServer->extensionSockets.size()) // 不需要发送，空着的
        return ;

    QJsonObject json;
    json.insert("cmd", cmd);
    json.insert("data", danmaku.toJson());
    QByteArray ba = QJsonDocument(json).toJson();

    foreach (QWebSocket* socket, webServer->extensionSockets)
    {
        if (!webServer->extensionCmdsMaps.contains(socket) || (!webServer->extensionCmdsMaps[socket].isEmpty() && !webServer->extensionCmdsMaps[socket].contains(cmd)))
            continue;
       socket->sendTextMessage(ba);
    }
}

void MainWindow::sendJsonToSockets(QString cmd, QJsonValue data, QWebSocket *socket)
{
    if (!socket && !webServer->extensionSockets.size()) // 不需要发送，空着的
        return ;

    QJsonObject json;
    json.insert("cmd", cmd);
    json.insert("data", data);
    QByteArray ba = QJsonDocument(json).toJson();
    sendTextToSockets(cmd, ba, socket);
}

void MainWindow::sendTextToSockets(QString cmd, QByteArray data, QWebSocket *socket)
{
    if (!socket && !webServer->extensionSockets.size())
    {
        qWarning() << "sendTextToSockets: 没有可发送的socket对象";
        return ;
    }

    // 这个data是包含了一个cmd和data
    if (socket)
    {
        qInfo() << "发送至指定socket：" << data;
        socket->sendTextMessage(data);
    }
    else
    {
        qInfo() << "发送至" << webServer->extensionSockets.size() << "个socket" << cmd << data;
        foreach (QWebSocket* socket, webServer->extensionSockets)
        {
            if (cmd.isEmpty() || (webServer->extensionCmdsMaps.contains(socket) && webServer->extensionCmdsMaps[socket].contains(cmd)))
            {
                socket->sendTextMessage(data);
            }
        }
    }
}

void MainWindow::sendMusicList(const SongList& songs, QWebSocket *socket)
{
    if (!webServer->sendSongListToSockets || (!socket && !webServer->extensionSockets.size()) || !musicWindow) // 不需要发送，空着的
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
        foreach (QWebSocket* socket, webServer->extensionSockets)
        {
            if (!webServer->extensionCmdsMaps.contains(socket) || webServer->extensionCmdsMaps[socket].contains("SONG_LIST"))
                socket->sendTextMessage(ba);
        }
    }
}

void MainWindow::sendLyricList(QWebSocket *socket)
{
    if (!webServer->sendLyricListToSockets || (!socket && !webServer->extensionSockets.size()) || !musicWindow)
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
        foreach (QWebSocket* socket, webServer->extensionSockets)
        {
            if (!webServer->extensionCmdsMaps.contains(socket) || webServer->extensionCmdsMaps[socket].contains("LYRIC_LIST"))
                socket->sendTextMessage(ba);
        }
    }
}

QString MainWindow::webCache(QString name) const
{
    return rt->dataPath + "cache/" + name;
}

void MainWindow::syncMagicalRooms()
{
    QString appVersion = GetFileVertion(QApplication::applicationFilePath()).trimmed();
    if (appVersion.startsWith("v") || appVersion.startsWith("V"))
        appVersion.replace(0, 1, "");

    get(serverPath + "client/active",
        {"room_id", ac->roomId, "user_id", ac->cookieUid, "up_id", ac->upUid,
         "room_title", ac->roomTitle, "username", ac->cookieUname, "up_name", ac->upName,
         "version", appVersion,
         "platform",
#ifdef Q_OS_WIN
         "windows",
#elif defined(Q_OS_MAC)
         "macos",
#elif defined(Q_OS_LINUX)
         "linux",
#endif
         "working", (cr->isWorking() ? "1" : "0"), "permission", snum(hasPermission()),
         "fans", snum(ac->currentFans), "guards", snum(ac->currentGuards.size()),
         "area", ac->parentAreaName + "/" + ac->areaName,
         "randkey", ac->csrf_token.toLatin1().toBase64()},
        [=](MyJson json) {
        // 检测数组
        json = json.data();
        QJsonArray roomArray = json.value("rooms").toArray();
        liveService->magicalRooms.clear();
        foreach (QJsonValue val, roomArray)
        {
            liveService->magicalRooms.append(val.toString());
        }

        // 检测新版
        QString latestVersion = json.value("latest_version").toString();
        if (latestVersion.startsWith("v") || latestVersion.startsWith("V"))
            latestVersion.replace(0, 1, "");
        rt->appNewVersion = latestVersion;

        if (compareVersion(latestVersion, appVersion) > 0) // 有更新
        {
            rt->appDownloadUrl = json.value("download_url").toString();
            ui->actionUpdate_New_Version->setText("有新版本：" + rt->appNewVersion);
            ui->actionUpdate_New_Version->setIcon(QIcon(":/icons/new_version"));
            ui->actionUpdate_New_Version->setEnabled(true);
            statusLabel->setText("有新版本：" + rt->appNewVersion);
            qInfo() << "有新版本：" << appVersion << "->" << rt->appNewVersion << rt->appDownloadUrl;

            QString packageUrl = json.s("package_url");
            if (!packageUrl.isEmpty())
            {
                if (ui->autoUpdateCheck->isChecked())
                {
                    // 自动更新，直接下载！
                    downloadNewPackage(latestVersion, packageUrl);

                    auto noti = new NotificationEntry("", "版本更新", "自动更新：" + appVersion + " -> " + latestVersion);
                    tip_box->createTipCard(noti);
                    localNotify("【自动更新】" + appVersion + " -> " + latestVersion);
                }
                else
                {
                    auto noti = new NotificationEntry("", "版本更新", appVersion + " -> " + latestVersion);
                    connect(noti, &NotificationEntry::signalCardClicked, this, [=]{
                        QDesktopServices::openUrl(rt->appDownloadUrl);
                    });
                    tip_box->createTipCard(noti);
                    localNotify("【有新版本】" + appVersion + " -> " + latestVersion);
                }
            }
            else
            {
                rt->appNewVersion = ""; // 不显示出来，不管它
            }
        }
        else
        {
            rt->appNewVersion = ""; // 没有更新
        }

        QString code = json.value("code").toString();
        if (!code.isEmpty())
        {
            cr->sendAutoMsg(code, LiveDanmaku());
        }

        QString msg = json.value("msg").toString();
        if (!msg.isEmpty())
        {
            QMessageBox::information(this, "神奇弹幕", msg);
        }

        if(json.value("auto_open").toBool())
        {
            QMessageBox::information(this, "版本更新", "您的版本已过旧，可能存在潜在问题，请尽快更新\n" + appVersion + " => " + rt->appNewVersion);
            QDesktopServices::openUrl(rt->appDownloadUrl);
        }

        if (json.value("force_update").toBool())
            QApplication::quit();
    });
}

#if defined(ENABLE_HTTP_SERVER)
void MainWindow::serverHandle(QHttpRequest *req, QHttpResponse *resp)
{
    if (req->method() == QHttpRequest::HttpMethod::HTTP_GET)
    {
        // GET方法不用获取数据
        return requestHandle(req, resp);
    }
    RequestBodyHelper *helper = new RequestBodyHelper(req, resp);
    helper->waitBody();
    connect(helper, SIGNAL(finished(QHttpRequest *, QHttpResponse *)), this, SLOT(requestHandle(QHttpRequest *, QHttpResponse *)));
}

QHash<QString, QString> parseUrlParams(const QString& paramString, bool urlDecode = true)
{
    QHash<QString, QString> params;
    QStringList sl = paramString.split("&"); // a=1&b=2的参数
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
        if (urlDecode)
        {
            params.insert(QByteArray::fromPercentEncoding(key.toUtf8()),
                          QByteArray::fromPercentEncoding(val.toUtf8()));
        }
        else
        {
            params.insert(key, val);
        }

    }
    return params;
}

void MainWindow::requestHandle(QHttpRequest *req, QHttpResponse *resp)
{
    // 解析参数
    const QString fullUrl = req->url().toString();
    QHash<QString, QString> params;
    QString urlPath = fullUrl; // 示例：/user/abc?param=123，不包括 域名
    if (fullUrl.contains("?"))
    {
        int pos = fullUrl.indexOf("?");
        urlPath = fullUrl.left(pos);
        params = parseUrlParams(fullUrl.right(fullUrl.length() - pos - 1));
    }

    // 路径
    if (urlPath.startsWith("/"))
        urlPath = urlPath.right(urlPath.length() - 1);
    if (urlPath.endsWith("/"))
        urlPath = urlPath.left(urlPath.length() - 1);
    urlPath = urlPath.trimmed();

//    qDebug() << "request ->" << urlPath;
    serverHandleUrl(urlPath, params, req, resp);
}

void MainWindow::serverHandleUrl(const QString &urlPath, QHash<QString, QString> &params, QHttpRequest *req, QHttpResponse *resp)
{
    QByteArray doc;
//    qInfo() << "http access:" << urlPath;

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
        return serverHandleUrl( urlPath + (urlPath.endsWith("/") || urlPath.isEmpty() ? "" : "/") + "index.html", params, req, resp);
    };

    // 判断文件类型
    QRegularExpressionMatch match;
    QString suffix;
    if (urlPath.indexOf(QRegularExpression("\\.(\\w+)$"), 0, &match) > -1)
        suffix = match.captured(1);

    auto isFileType = [=](QString types) -> bool {
        if (suffix.isEmpty())
            return false;
        return types.indexOf(suffix) > -1;
    };

    // 内容类型
    QString contentType = suffix.isEmpty() ? "text/html"
                                           : webServer->contentTypeMap.value(suffix, "application/octet-stream");

    // 开始特判
    // ========== 各类接口 ==========
    if (urlPath.startsWith("api/"))
    {
        doc = processApiRequest(urlPath.right(urlPath.length() - 4), params, &contentType, req, resp);
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
             && !QFileInfo(webServer->wwwDir.absoluteFilePath("index.html")).exists())
    {
        doc = "<html><head><title>神奇弹幕</title></head><body><h1>服务开启成功！</h1></body></html>";
    }
    else if (suffix.isEmpty() && QDir().exists(webServer->wwwDir.absoluteFilePath(urlPath))) // 没有后缀名，也没有特判的
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
        QString filePath = webServer->wwwDir.absoluteFilePath(urlPath);
        QFile file(filePath);
        if (!file.exists())
        {
            qWarning() << "文件：" << filePath << "不存在";
            return errorStr("File " + urlPath + " Not Found", QHttpResponse::STATUS_NOT_FOUND);
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
    if (contentType.startsWith("text/") && !contentType.contains("charset"))
        contentType += ";charset=utf-8";
    resp->setHeader("Content-Type", contentType);
    resp->setHeader("Content-Length", snum(doc.size()));
    resp->setHeader("Access-Control-Allow-Origin", "*");
    resp->writeHead(QHttpResponse::STATUS_OK);
    resp->write(doc);
    resp->end();
}

/// 一些header相关的
/// @param url 不包括前缀api/，直达动作本身
/// 示例地址：http://__DOMAIN__:__PORT__/api/header?uid=123456
QByteArray MainWindow::processApiRequest(QString url, QHash<QString, QString> params, QString* contentType, QHttpRequest *req, QHttpResponse *resp)
{
    QByteArray ba;
    if (url == "header") // 获取头像 /api/header?uid=1234565
    {
        *contentType = "image/jpeg";
        if (!params.contains("uid"))
            return ba;

        QString uid = params.value("uid");
        QString filePath = webCache("header_" + uid);
        if (!isFileExist(filePath))
        {
            // 获取封面URL并下载封面
            QString url = liveService->getApiUrl(UserHead, uid.toLongLong());
            NetUtil::downloadWebFile(url, filePath);
        }

        // 返回文件
        QFile f(filePath);
        f.open(QIODevice::ReadOnly);
        ba = f.readAll();
        f.close();
    }
    else if (url == "netProxy") // 网络代理，解决跨域问题 /api/netProxy?url=https://www.baidu.com(urlEncode)&referer=
    {
        QString url = params.value("url", "");
        auto method = req->method();
        qInfo() << "代理URL：" << url;

        if (url.isEmpty())
            return QByteArray("参数 URL 不能为空");

        QNetworkAccessManager manager;
        QNetworkRequest request(url);

        // 转发headers
        const auto& headers = req->headers();
        QStringList igns { "host", "accept-encoding" };
        for (auto it = headers.begin(); it != headers.end(); it++)
        {
            if (igns.contains(it.key()))
                continue;
             request.setRawHeader(it.key().toUtf8(), it.value().toUtf8());
        }

        // 读取cookies
        if (!headers.contains("cookies"))
            setUrlCookie(url, &request);

        // 遍历所有未知的参数，设置到header
        params.remove("url");
        for (auto it = params.begin(); it != params.end(); it++)
        {
            request.setRawHeader(it.key().toUtf8(), QByteArray::fromPercentEncoding(it.value().toUtf8()));
        }

        // 异步返回
        QNetworkReply *reply = nullptr;
        QEventLoop loop;
        if (method == QHttpRequest::HttpMethod::HTTP_GET) // get
        {
            reply = manager.get(request);
            QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit())); //请求结束并下载完成后，退出子事件循环
            loop.exec(); //开启子事件循环
        }
        else // post delete put 等
        {
            QByteArray data = req->body();
            if (method == QHttpRequest::HttpMethod::HTTP_POST)
                reply = manager.post(request, data);
            else if (method == QHttpRequest::HttpMethod::HTTP_PUT)
                reply = manager.put(request, data);
            else
                reply = manager.post(request, data);
            QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit())); //请求结束并下载完成后，退出子事件循环
            loop.exec(); //开启子事件循环
        }

        if (reply->hasRawHeader("content-type"))
        {
            *contentType = reply->rawHeader("content-type");
        }
        ba = reply->readAll();
        // qInfo() << "reply:" << ba;
        reply->deleteLater();
    }
    else if (url == "event") // 模拟事件： /api/event?event=SEND_GIFT&data=[urlEncode(json)]
    {
        /* if (!ui->allowWebControlCheck->isChecked())
        {
            qWarning() << "api/event: 需要解锁安全限制";
            ba = "{ \"cmd\": \"event\", \"code\": \"-1\", \"msg\": \"需要解锁安全限制\" }";
            return ba;
        } */

        *contentType = "application/json";
        QString eventName = params.value("event");
        QByteArray data;
        if (eventName.isEmpty())
        {
            qWarning() << "api/event: ignore empty event";
            ba = "{ \"cmd\": \"event\", \"code\": \"1\", \"msg\": \"ignore empty event\" }";
            return ba;
        }
        if (params.contains("data"))
        {
            // 可以直接使用get的形式来设置data（不能包含特殊字符，尽量URL编码）
            data = QByteArray::fromPercentEncoding(params.value("data").toUtf8());
        }
        else
        {
            // 也可以是post的形式发送json过来
            data = req->body();
        }

        MyJson json(data);

        // 判断权限
        {
            LiveDanmaku _dmk;
            _dmk.setText(eventName);
            _dmk.with(json);
            if (cr->isFilterRejected("FILTER_API_EVENT", _dmk))
            {
                qInfo() << "过滤器阻止Event：" << eventName;
                ba = "{ \"cmd\": \"event\", \"code\": \"2\", \"msg\": \"prevent event by filter\" }";
                return ba;
            }
        }

        qInfo() << "模拟事件：" << eventName << json;
        LiveDanmaku danmaku = LiveDanmaku::fromDanmakuJson(json);
        if (!json.contains("extra"))
            danmaku.with(json);
        triggerCmdEvent(eventName, danmaku);
        ba = "{ \"cmd\": \"event\", \"code\": \"0\", \"msg\": \"ok\" }";
    }
    else if (url == "readConfig" || url == "getConfig") // 获取所有或者单个配置
    {
        if (!ui->allowWebControlCheck->isChecked())
        {
            qWarning() << "api/" + url + ": 需要解锁安全限制";
            ba = "{ \"cmd\": \"api\", \"code\": \"-1\", \"msg\": \"需要解锁安全限制\" }";
            return ba;
        }

        QString format = params.value("format").toLower();
        QString key = QByteArray::fromPercentEncoding(params.value("key", "").toUtf8());
        if (format == "json") // 读取所有的key-value对，并且转换成JSON格式
        {
            MyJson json;
            if (!key.isEmpty())
            {
                json.insert(key, us->value(key).toString());
            }
            else
            {
                auto keys = us->allKeys();
                foreach (auto k, keys)
                {
                    json.insert(k, us->value(k).toJsonValue());
                }
            }
            ba = json.toBa();
        }
        else if (key.isEmpty())
        {
            qInfo() << "获取所有配置";
            ba = readTextFileAutoCodec(rt->dataPath + "settings.ini").toUtf8();
        }
        else
        {
            qInfo() << "获取配置：" << key;
            ba = us->value(key).toByteArray();
        }
    }
    else if (url == "writeConfig" || url == "setConfig") // 设置单个配置
    {
        if (!ui->allowWebControlCheck->isChecked())
        {
            qWarning() << "api/" + url + ": 需要解锁安全限制";
            ba = "{ \"cmd\": \"api\", \"code\": \"-1\", \"msg\": \"需要解锁安全限制\" }";
            return ba;
        }

        // get的参数
        QString key = params.value("key", "");
        QString value = params.value("value", "");
        bool autoReload = !params.value("reload").isEmpty();

        // post的参数
        if (key.isEmpty())
        {
            const QByteArray& body = req->body();
            if (!body.isEmpty())
            {
                MyJson json(body);
                if (!json.isEmpty()) // post json
                {
                    key = json.value("key").toString();
                    value = json.value("value").toString();
                    autoReload = json.value("reload").toBool();
                }
                else // post raw
                {
                    qDebug() << "解析URL参数：" << body;
                    // 一些body不会进行URL编码，这样会导致多一次解码，容易出问题
                    auto params = parseUrlParams(body, false);
                    key = params.value("key", "");
                    value = params.value("value", "");
                    autoReload = !params.value("reload").isEmpty();
                }
            }
            else
            {
                qWarning() << "设置配置：post 的 body 为空";
            }
        }

        if (key.isEmpty())
        {
            return ba = "未包含指定参数“key”";
        }
        qInfo() << "设置配置：" << key << "=" << value;
        us->set(key, value);
        us->sync();

        if (autoReload)
        {
            qInfo() << "重新读取配置";
            readConfig();
        }
        *contentType = "application/json";
        ba = "{\"code\": 0}";
    }
    else if (url == "reloadConfig")
    {
        if (!ui->allowWebControlCheck->isChecked())
        {
            qWarning() << "api/" + url + ": 需要解锁安全限制";
            ba = "{ \"cmd\": \"api\", \"code\": \"-1\", \"msg\": \"需要解锁安全限制\" }";
            return ba;
        }
        QTimer::singleShot(100, this, [=]{
            qInfo() << "重新读取配置";
            readConfig();
        });
        *contentType = "application/json";
        ba = "{\"code\": 0}";
    }

    return ba;
}
#endif

void MainWindow::processServerVariant(QByteArray &doc)
{
    doc.replace("__DOMAIN__", webServer->serverDomain.toUtf8())
            .replace("__PORT__", snum(webServer->serverPort).toUtf8())
            .replace("__WS_PORT__", snum(webServer->serverPort+1).toUtf8());
}

void MainWindow::pullRoomShieldKeyword()
{
    if (ac->roomId.isEmpty() || ac->cookieUid.isEmpty()) // 没登录
        return ;
    if (!ui->syncShieldKeywordCheck->isChecked()) // 没开启
        return ;

    // 获取直播间的屏蔽词
    // 先获取这个是为了判断权限，不是房管的话直接ban掉
    bool ok;
    QStringList roomList = liveService->getRoomShieldKeywordsAsync(&ok);
    if (!ok)
        return;

    // 获取云端的（根据上次同步时间）
    qint64 time = us->value("sync/shieldKeywordTimestamp_" + ac->roomId, 0).toLongLong();
    MyJson cloudSK(NetUtil::getWebData(serverPath + "keyword/getNewerShieldKeyword?time=" + snum(time)));
    if (cloudSK.code() != 0)
        return showError("获取云端屏蔽词失败", cloudSK.msg());
    MyJson cloudData = cloudSK.data();
    QJsonArray addedArray = cloudData.a("keywords"), removedArray = cloudData.a("removed");
    qInfo() << "云端添加：" << addedArray.size() << "  云端删除：" << removedArray.size() << "   时间戳" << time;
    if (!addedArray.size() && !removedArray.size())
        return ;
    // 没有可修改的，后面全部跳过
    // 也不更新时间了，因为有可能是网络断开，或者服务端挂掉
    us->setValue("sync/shieldKeywordTimestamp_" + ac->roomId, QDateTime::currentSecsSinceEpoch());

    // 添加新的
    foreach (auto k, addedArray)
    {
        QString s = k.toString();
        if (roomList.contains(s))
        {
            qInfo() << "跳过已存在：" << s;
            continue;
        }

        qInfo() << "添加直播间屏蔽词：" << s;
        localNotify("添加屏蔽词：" + s);
        liveService->addRoomShieldKeywordsAsync(s);
    }

    // 删除旧的
    foreach (auto k, removedArray)
    {
        QString s = k.toString();
        if (!roomList.contains(s))
        {
            qInfo() << "跳过不存在：" << s;
            continue;
        }

        qInfo() << "移除直播间屏蔽词：" << s;
        localNotify("移除屏蔽词：" + s);
        liveService->removeRoomShieldKeywordAsync(s);
    }
}

void MainWindow::addCloudShieldKeyword(QString keyword)
{
    // 添加到直播间
    liveService->addRoomShieldKeywordsAsync(keyword);

    // 添加到云端
    qInfo() << "添加云端屏蔽词：" << keyword;
    MyJson json = MyJson(NetUtil::postWebData(serverPath + "keyword/addShieldKeyword",
                {"keyword", keyword, "userId", ac->cookieUid, "username", ac->cookieUname, "roomId", ac->roomId}, ac->userCookies).toLatin1());
    if (json.code() != 0)
        qWarning() << "添加云端屏蔽词失败：" << json;
}

void MainWindow::downloadNewPackage(QString version, QString packageUrl)
{
    // 判断名字
    if (QCoreApplication::applicationName() != "神奇弹幕")
    {
        qWarning() << "非默认APP名字，不自动更新";
        return ;
    }

    // 开始下载
    static bool downloaded = false;
    if (!downloaded)
        downloaded = true;
    else
        return ;
    qInfo() << "下载：" << version << packageUrl;
#ifdef Q_OS_WIN
    QString pkgPath = QApplication::applicationDirPath() + "/update.zip";
    if (isFileExist(pkgPath))
    {
        qInfo() << "待更新安装包已存在，等待安装";
        return ;
    }
    QProcess process(this);
    QStringList list{ "-d", packageUrl, pkgPath};
    if (process.startDetached(UPDATE_TOOL_NAME, list))
    {
        qInfo() << "调用更新程序下载成功";
        return ;
    }
    qCritical() << "无法调用更新程序，请手动下载";
    QDesktopServices::openUrl(QUrl(packageUrl));
#else
    QDesktopServices::openUrl(QUrl(packageUrl));
#endif
}
