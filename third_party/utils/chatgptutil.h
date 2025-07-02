#ifndef CHATGPTUTIL_H
#define CHATGPTUTIL_H

#include "netutil.h"
#include <QFuture>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QtConcurrent/QtConcurrent>
#include "usersettings.h"

#define LOG_TAG "ai.chatgpt"

struct ChatBean
{
    QString role;
    QString message;
    int length = 0;
    qint64 timestamp = 0;
    int total_token = 0;

    ChatBean(QString self, QString message) : role(self), message(message)
    {
        length = message.length();
        timestamp = QDateTime::currentSecsSinceEpoch();
    }
};

class ChatGPTUtil : public QThread
{
    Q_OBJECT
signals:
    void signalResponseText(const QString& text);
    void signalResponseJson(const QJsonObject& text);
    void signalResponseError(const QByteArray& text);
    void signalRequestStarted();
    void signalResponseFinished();

    void signalStreamStarted();
    void signalStreamFinished();
    void signalStreamText(const QString& text);
    void signalStreamJson(const QJsonObject& json);

    void signalStopThread();

public:
    ChatGPTUtil(QObject* parent = nullptr) : QThread(parent)
    {

    }

    void setStream(bool stream)
    {
        this->use_stream = stream;
    }

    bool isUseStream() const
    {
        return use_stream;
    }

    bool isWaiting() const
    {
        return waiting;
    }

    void getResponse(const QString& paramText, QJsonObject extraKey = QJsonObject())
    {
        QJsonObject json;
        json.insert("model", us->chatgpt_model_name);
        QJsonArray array;
        QJsonObject rc;
        rc.insert("role", "user");
        rc.insert("content", paramText);
        array.append(rc);
        json.insert("messages", array);
        json.insert("temperature", 0.7);

        foreach (QString key, extraKey.keys())
        {
            json.insert(key, extraKey.value(key));
        }

        getResponse(json);
    }

    void getResponse(QList<ChatBean> chats, QJsonObject extraKey = QJsonObject())
    {
        QJsonObject json;
        json.insert("model", us->chatgpt_model_name);
        QJsonArray array;
        for (int i = 0; i < chats.size(); i++)
        {
            QJsonObject rc;
            rc.insert("role", chats[i].role);
            rc.insert("content", chats[i].message);
            array.append(rc);
        }
        json.insert("messages", array);
        json.insert("temperature", 0.7);
        // json.insert("max_tokens", us->chatgpt_max_token_count);

        foreach (QString key, extraKey.keys())
        {
            json.insert(key, extraKey.value(key));
        }

        getResponse(json);
    }

    void getResponse(const QJsonObject& paramJson)
    {
        if (us->chatgpt_endpiont.isEmpty())
        {
            qWarning() << "未设置API地址";
            emit signalResponseFinished();
            return ;
        }

        if (us->open_ai_key.isEmpty())
        {
            qWarning() << "未设置OpenAI秘钥";
            emit signalResponseFinished();
            return ;
        }

        url = us->chatgpt_endpiont;
//        if (us->open_ai_key.startsWith("fk"))
//            url = "https://stream.api2d.net/v1/chat/completions";
//        else if (us->open_ai_key.startsWith("sk"))
//            url = "https://api.openai.com/v1/chat/completions";
//        else
//        {
//            // qWarning() << "设置的不正确的key：" << us->open_ai_key;
//        }

        this->param_json = paramJson;

        this->start();
    }

    void parseResponse(QByteArray ba)
    {
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(ba, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qCritical() << ("读取ChatGPT Stream失败：" + ba);
            emit signalResponseError("无法解析：" + ba);
            return ;
        }
        QJsonObject json = document.object();
        QJsonArray choices = json.value("choices").toArray();
        if (choices.size())
        {
            QJsonObject choice0 = choices.first().toObject();
            QJsonObject message;
            if (use_stream)
            {
                message = choice0.value("delta").toObject();
            }
            else
            {
                message = choice0.value("message").toObject();
            }

            if (message.isEmpty())
            {
                if (choice0.value("finish_reason").toString() != "stop")
                {
                    qCritical() << ("无法获取message：" + QString(ba));
                    emit signalResponseError("无法获取message：" + ba);
                }
            }
            QString text = message.value("content").toString();
            if (use_stream)
            {
                // logd("ChatGPT流文本：" + text);
                emit signalStreamText(text);
            }
            else
            {
                qDebug() << ("ChatGPT回复文本：" + text.left(300));
                emit signalResponseText(text);
            }
        }
        else
        {
            qDebug() << ("无法解析ChatGPT回复：" + QString(ba));
            // 解析错误
            if (json.contains("error") && json.value("error").isObject())
                json = json.value("error").toObject();
            if (json.contains("message"))
            {
                int code = json.value("code").toInt();
                QString type = json.value("type").toString();
                emit signalResponseError((json.value("message").toString() + "\n\n错误码：" + QString::number(code) + "  " + type).toUtf8());
            }
            else
            {
                emit signalResponseError(ba);
            }
        }
    }

    void run() override
    {
        waiting = true;
        QNetworkRequest request;
        request.setUrl(url);
        request.setRawHeader("Authorization", "Bearer " + us->open_ai_key.toUtf8());
        request.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/json"));

        if (use_stream)
        {
            param_json.insert("stream", true);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
            request.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
#else
            request.setAttribute(QNetworkRequest::RedirectPolicyAttribute,
                                 QNetworkRequest::NoLessSafeRedirectPolicy);
#endif
            request.setAttribute(QNetworkRequest::CacheLoadControlAttribute, QNetworkRequest::AlwaysNetwork); // Events shouldn't be cached
        }

        QNetworkAccessManager manager;
        QNetworkReply *reply;

        reply = manager.post(request, QJsonDocument(param_json).toJson());

        emit signalRequestStarted();

        if (use_stream) // 使用流的形式
        {
            connect(reply, &QNetworkReply::readyRead, this, [=]() {
                if (force_stop)
                {
                    qWarning() << "已阻止结束的GPT线程";
                    return;
                }
                QString ba = QString(reply->readAll());
                QStringList list = ba.split("\n\n");
                foreach (auto str, list)
                {
                    QString content = str.simplified().remove(0, 5).trimmed();
                    if (content.isEmpty())
                        continue;
                    if (content == "[DONE]") // 最后一个
                        return ;
                    parseResponse(content.toUtf8());
                }
            });
        }

        QEventLoop loop;
        connect(this, SIGNAL(signalStopThread()), &loop, SLOT(quit()));
        QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit())); //请求结束并下载完成后，退出子事件循环
        loop.exec(); // 开启子事件循环

        if (force_stop)
        {
            force_stop = false;
            qDebug() << ("ChatGPT强制结束线程");
            emit signalResponseFinished();
            return ;
        }

        if (!use_stream)
        {
            QByteArray ba = reply->readAll();
            parseResponse(ba);
        }
        waiting = false;
        emit signalResponseFinished();
    }

    void stop()
    {
        force_stop = true;
        emit signalStopThread();
        waiting = false;
    }

    void stopAndDelete()
    {
        if (!waiting)
        {
            deleteLater();
        }
        else
        {
            // connect(this, SIGNAL(signalResponseFinished()), this, SLOT(deleteLater()));
            stop();
        }
    }

private:
    QString url;
    QJsonObject param_json;
    NetResultFuncType callback;
    bool use_stream = false;
    bool waiting = false;
    bool force_stop = false; // 多线程终止
};

#endif // CHATGPTUTIL_H
