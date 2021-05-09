#ifndef NETUTIL_H
#define NETUTIL_H

#include <QObject>
#include <QThread>
#include <QEventLoop>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrl>
#include <QTextCodec>
#include <QFile>
#include <initializer_list>
#include <QRegularExpression>

typedef std::function<void(QString)> const NetResultFuncType;

/**
 * 网络操作做工具类，读取网络源码
 */
class NetUtil : public QObject
{
    Q_OBJECT
public:
    static QByteArray getWebData(QString uri)
    {
        QUrl url(uri);
        QNetworkAccessManager manager;
        QEventLoop loop;
        QNetworkReply *reply;

        reply = manager.get(QNetworkRequest(url));
        QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit())); //请求结束并下载完成后，退出子事件循环
        loop.exec(); //开启子事件循环

        QByteArray code_content(reply->readAll().data());

        reply->deleteLater();
        return code_content;
    }

    static QString postWebData(QString uri, QString data)
    {
        QUrl url(uri);
        QNetworkAccessManager manager;
        QEventLoop loop;
        QNetworkReply *reply;

        reply = manager.post(QNetworkRequest(url), data.toLatin1());
        QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit())); //请求结束并下载完成后，退出子事件循环
        loop.exec(); //开启子事件循环

        QString code_content(reply->readAll().data());
        reply->deleteLater();
        return code_content;
    }

    static QString downloadWebFile(QString uri, QString path)
    {
        QNetworkAccessManager manager;
        QEventLoop loop;
        QNetworkReply *reply;

        reply = manager.get(QNetworkRequest(QUrl(uri)));
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
            QString str = QString(reply->readAll().data());
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
            QString str = QString(reply->readAll().data());
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
