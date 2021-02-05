#include "mainwindow.h"

void MainWindow::serverHandle(QHttpRequest *req, QHttpResponse *resp)
{
    QString path = req->path(); // 示例：/user/abc
    resp->setHeader("Content-Type", "text/html;charset=utf-8");
    QByteArray doc;

    auto errorResp = [=](QByteArray err){
        resp->setHeader("Content-Length", snum(err.size()));
        resp->writeHead(400);
        resp->write(err);
        resp->end();
    };

    if (path.startsWith("/danmaku")) // 弹幕姬
    {
        if (!danmakuWindow)
            return errorResp("弹幕姬未开启");
    }
    else if (path.startsWith("/order")) // 点歌姬
    {
        if (!musicWindow)
            return errorResp("点歌姬未开启");
    }
    else if (path == "/favicon.ico")
    {
        QBuffer buffer(&doc);
        buffer.open(QIODevice::WriteOnly);
        QPixmap(":/icons/star").save(&buffer,"PNG");
    }
    else // 显示
    {
        doc = "<html><head><title>神奇弹幕</title></head><body><h1>服务开启成功！</h1></body></html>";
    }

    resp->setHeader("Content-Length", snum(doc.size()));
    resp->writeHead(200);
    resp->write(doc);
    resp->end();
}
