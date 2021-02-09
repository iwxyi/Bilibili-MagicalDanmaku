#include "mainwindow.h"

void MainWindow::serverHandle(QHttpRequest *req, QHttpResponse *resp)
{
    QString urlPath = req->path(); // 示例：/user/abc
    if (urlPath.startsWith("/"))
        urlPath = urlPath.right(urlPath.length() - 1);
    if (urlPath.endsWith("/"))
        urlPath = urlPath.left(urlPath.length() - 1);
    urlPath = urlPath.trimmed();
    // qDebug() << "request path:" << urlPath;

    resp->setHeader("Content-Type", "text/html;charset=utf-8");
    QByteArray doc;

    auto errorResp = [=](QByteArray err){
        resp->setHeader("Content-Length", snum(err.size()));
        resp->writeHead(400);
        resp->write(err);
        resp->end();
    };

    if (urlPath == "danmaku") // 弹幕姬
    {
        if (!danmakuWindow)
            return errorResp("弹幕姬未开启");
    }
    else if (urlPath == "music") // 点歌姬
    {
        if (!musicWindow)
            return errorResp("点歌姬未开启");
    }
    else if (urlPath == "favicon.ico")
    {
        QBuffer buffer(&doc);
        buffer.open(QIODevice::WriteOnly);
        QPixmap(":/icons/star").save(&buffer,"PNG");
    }
    else if (urlPath.isEmpty()) // 显示
    {
        doc = "<html><head><title>神奇弹幕</title></head><body><h1>服务开启成功！</h1></body></html>";
    }
    else // 设置文件
    {
        QString filePath = wwwDir.absoluteFilePath(urlPath);
        QFile file(filePath);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            doc = file.readAll();
            file.close();
        }
    }

    resp->setHeader("Content-Length", snum(doc.size()));
    resp->writeHead(200);
    resp->write(doc);
    resp->end();
}
