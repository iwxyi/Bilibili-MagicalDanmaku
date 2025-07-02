#ifndef NETUTIL_H
#define NETUTIL_H

#include <QObject>
#include <QThread>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QFile>
#include <initializer_list>
#include <QRegularExpression>
#include <QJsonObject>
#include <QJsonDocument>
#include <QProcess>
#include <QFuture>
#include <QtConcurrent/QtConcurrent>

typedef std::function<void(QString)> const NetResultFuncType;

/**
 * 网络操作做工具类，读取网络源码
 */
class NetUtil : public QObject
{
    Q_OBJECT
public:
    static QByteArray getWebData(QString uri, QVariant cookies = QVariant())
    {
        QUrl url(uri);
        QNetworkAccessManager manager;
        QEventLoop loop;
        QNetworkReply *reply;
        QNetworkRequest request(url);
        if (!cookies.isNull())
            request.setHeader(QNetworkRequest::CookieHeader, cookies);

        reply = manager.get(request);
        QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit())); //请求结束并下载完成后，退出子事件循环
        loop.exec(); //开启子事件循环

        QByteArray code_content(reply->readAll());

        reply->deleteLater();
        return code_content;
    }

    static QString postWebData(QString uri, QString data, QVariant cookies = QVariant())
    {
        QUrl url(uri);
        QNetworkAccessManager manager;
        QEventLoop loop;
        QNetworkReply *reply;
        QNetworkRequest request(url);
        if (!cookies.isNull())
            request.setHeader(QNetworkRequest::CookieHeader, cookies);
        request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/x-www-form-urlencoded"));

        reply = manager.post(request, data.toLatin1());
        QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit())); //请求结束并下载完成后，退出子事件循环
        loop.exec(); //开启子事件循环

        QString code_content(reply->readAll());
        reply->deleteLater();
        return code_content;
    }

    static QByteArray postWebData(QString uri, QJsonObject json, QVariant cookies = QVariant())
    {
        QUrl url(uri);
        QNetworkAccessManager manager;
        QEventLoop loop;
        QNetworkReply *reply;
        QNetworkRequest request(url);
        if (!cookies.isNull())
            request.setHeader(QNetworkRequest::CookieHeader, cookies);
        request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));


        reply = manager.post(request, QJsonDocument(json).toJson());
        QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit())); //请求结束并下载完成后，退出子事件循环
        loop.exec(); //开启子事件循环

        QByteArray code_content(reply->readAll());
        reply->deleteLater();
        return code_content;
    }

    static QString postWebData(QString uri, QStringList params, QVariant cookies = QVariant())
    {
        QString data;
        for (int i = 0; i < params.size(); i++)
        {
            if (i & 1) // 用户数据
                data += QUrl::toPercentEncoding(params.at(i));
            else // 固定变量
                data += (i==0?"":"&") + params.at(i) + "=";
        }

        return postWebData(uri, data, cookies);
    }

    static QString downloadWebFile(QString uri, QString path, QVariant cookies = QVariant())
    {
        QNetworkAccessManager manager;
        QEventLoop loop;
        QNetworkReply *reply;
        QNetworkRequest request(uri);
        if (!cookies.isNull())
            request.setHeader(QNetworkRequest::CookieHeader, cookies);

        reply = manager.get(request);
        QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit())); //请求结束并下载完成后，退出子事件循环
        loop.exec(); //开启子事件循环

        QFile file(path);
        if (!file.open(QFile::WriteOnly))
        {
            qDebug() << "写入文件失败" << path;
            reply->deleteLater();
            return "";
        }
        QByteArray data = reply->readAll();
        if (!data.isEmpty())
        {
            qint64 write_bytes = file.write(data);
            file.flush();
            if (write_bytes != data.size())
                qDebug() << "写入文件大小错误" << write_bytes << "/" << data.size();
        }

        reply->deleteLater();
        return path;
    }

    static QByteArray getWebFile(QString uri)
    {
        QNetworkAccessManager manager;
        QEventLoop loop;
        QNetworkReply *reply;

        reply = manager.get(QNetworkRequest(QUrl(uri)));
        QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit())); //请求结束并下载完成后，退出子事件循环
        loop.exec(); //开启子事件循环

        QByteArray ba = reply->readAll();
        reply->deleteLater();
        return ba;
    }

    // =======================================================================

    static void get(QString uri, NetResultFuncType func)
    {
        NetUtil* net = new NetUtil(uri);
        connect(net, &NetUtil::finished, net, [=](QString s){
            func(s);
        });
    }

    static void get(QString uri, QString params, NetResultFuncType func)
    {
        NetUtil* net = new NetUtil();
        if (uri.endsWith("?") || params.startsWith("?"))
            uri = uri + params;
        else
            uri = uri + "?" + params;
        connect(net, &NetUtil::finished, net, [=](QString s){
            func(s);
            net->deleteLater();
        });
        net->get(uri);
    }

    static void get(QString uri, QStringList params, NetResultFuncType func)
    {
        NetUtil* net = new NetUtil();
        QString data;
        for (int i = 0; i < params.size(); i++)
        {
            if (i & 1) // 用户数据
                data += QUrl::toPercentEncoding(params.at(i));
            else // 固定变量
                data += (i==0?"":"&") + params.at(i) + "=";
        }
        connect(net, &NetUtil::finished, net, [=](QString s){
            func(s);
            net->deleteLater();
        });
        net->get(uri + "?" + data);
    }

    static void post(QString uri,NetResultFuncType func)
    {
        NetUtil* net = new NetUtil(uri);
        connect(net, &NetUtil::finished, net, [=](QString s){
            func(s);
        });
    }

    static void post(QString uri, QString params,NetResultFuncType func)
    {
        NetUtil* net = new NetUtil(uri, params);
        connect(net, &NetUtil::finished, net, [=](QString s){
            func(s);
        });
    }

    static void post(QString uri, QStringList params,NetResultFuncType func)
    {
        NetUtil* net = new NetUtil(uri, params);
        connect(net, &NetUtil::finished, net, [=](QString s){
            func(s);
        });
    }

    /**
     * 从文本中提取特定格式的列表（例如 <li>(.+?)</li>）
     */
    static QStringList extractList(QString text, QString pat)
    {
        QRegularExpression rx(pat);
        if (text.indexOf(rx))
            return rx.match(text).capturedTexts();
        else
            return QStringList();
    }

    /**
     * 从文本中提取一条特定格式字符串
     */
    static QString extractStr(QString text, QString pat)
    {
        QStringList list = extractList(text, pat);
        if (list.size())
            return list.first();
        else
            return QString();
    }

    /**
     * 从文本中提取一条特定格式字符串，排除第一个（如果有两个及以上的话）
     */
    static QString extractOne(QString text, QString pat)
    {
        QStringList list = extractList(text, pat);
        if (list.size() >= 2)
            return list.at(1);
        else if (list.size())
            return list.first();
        else
            return QString();
    }

    // =======================================================================
    // 判断是否联网了（阻塞主线程）
    static bool checkPublicNet(bool out = false)
    {
        QString url = out ? "www.google.com" : "www.baidu.com";
        QString networkCmd = "ping " + url + " -n 2 -w 500";
        QProcess process;
        process.start(networkCmd);
        process.waitForFinished();
        QString result = process.readAll();
        return result.contains("TTL=");
    }

    // 判断是否联网（不阻塞）
    static QFuture<bool> checkPublicNetThread(bool out = false)
    {
        QString url = out ? "www.google.com" : "www.baidu.com";
        QString networkCmd = "ping " + url + " -n 2 -w 1000";
        auto check = [=]{
            QProcess process;
            process.start(networkCmd);
            process.waitForFinished();
            QByteArray result = process.readAll();
            return result.contains("TTL=");
        };
        //放在线程中，防止界面卡住
        QFuture<bool> future= QtConcurrent::run(check);

        return future;
    }



public:
    NetUtil() {}

    NetUtil(QString uri)
    {
        get(uri);
        connect(this, &NetUtil::finished, [=]{
            this->deleteLater();
        });
    }

    NetUtil(QString uri, QString param)
    {
        post(uri, param);
        connect(this, &NetUtil::finished, [=]{
            this->deleteLater();
        });
    }

    NetUtil(QString uri, QStringList params)
    {
        QString data;
        for (int i = 0; i < params.size(); i++)
        {
            if (i & 1) // 用户数据
                data += QUrl::toPercentEncoding(params.at(i));
            else // 固定变量
                data += (i==0?"":"&") + params.at(i) + "=";
        }
        post(uri, data);
        connect(this, &NetUtil::finished, [=]{
            this->deleteLater();
        });
    }

public:

    void get(QString uri)
    {
        QNetworkRequest request;
        request.setUrl(QUrl(uri));
        request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/x-www-form-urlencoded"));
        QNetworkAccessManager *manager = new QNetworkAccessManager(this);

        QNetworkReply *reply = manager->get(request);
        QObject::connect(reply, &QNetworkReply::finished, [=]{
            QString str = reply->readAll();
            emit finished(str);
            reply->deleteLater();
            manager->deleteLater();
        });
    }

    void post(QString uri, QString data)
    {
        QNetworkRequest request;
        request.setUrl(QUrl(uri));
        request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/x-www-form-urlencoded"));
        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
#ifdef NET_DEBUG
qDebug() << "网址 post ：" << uri << data;
#endif
        auto body = data.toLatin1();
        QNetworkReply *reply = manager->post(request, body);

        QObject::connect(reply, &QNetworkReply::finished, [=]{
            QString str = reply->readAll();
#ifdef NET_DEBUG
            qDebug() << "返回结果：" << str; // 注意：要是传回来的内容太长（超过3万6左右），qDebug不会输出。这是可以输出size()来查看
#endif
            emit finished(str);
            reply->deleteLater();
            manager->deleteLater();
        });
    }

    void download(QString uri, QString path)
    {
        QNetworkAccessManager* manager = new QNetworkAccessManager;
        QEventLoop loop;
        QNetworkReply *reply;

        reply = manager->get(QNetworkRequest(QUrl(uri)));

        connect(reply, &QNetworkReply::downloadProgress, [=](qint64 recv, qint64 total){
            emit progress(recv, total);
        });

        QObject::connect(reply, &QNetworkReply::finished, [=]{
            QFile file(path);
            if (!file.open(QFile::WriteOnly))
            {
                qDebug() << "文件打开失败" << path;
                reply->deleteLater();
                manager->deleteLater();
                emit finished("");
                return ;
            }

            QByteArray data = reply->readAll();
            if (!data.isEmpty())
            {
                qint64 write_bytes = file.write(data);
                file.flush();
                if (write_bytes != data.size())
                    qDebug() << "写入文件大小错误" << write_bytes << "/" << data.size();
            }

            emit finished(path);
            reply->deleteLater();
            manager->deleteLater();
        });
    }

signals:
    void progress(qint64, qint64);
    void finished(QString);
};

#endif // NETUTIL_H
