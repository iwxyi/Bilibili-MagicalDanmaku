#ifndef TX_NLP_H
#define TX_NLP_H

#include <QMessageAuthenticationCode>
#include <QEventLoop>
#include "netinterface.h"

class TxNlp
{
private:
    TxNlp() {}
public:
    /**
     * 单例模式
     */
    static TxNlp* instance()
    {
        if (!txNlp)
            txNlp = new TxNlp();
        return txNlp;
    }

    void setSecretId(const QString& id) { this->secretId = id.toLocal8Bit(); }

    void setSecretKey(const QString& key) { this->secretKey = key.toLocal8Bit(); }

    /**
     * NLP：https://cloud.tencent.com/document/product/271/39416
     * 公共参数：https://cloud.tencent.com/document/api/271/35487（Hash256参数是反的）
     * 参数调试：https://console.cloud.tencent.com/api/explorer?Product=nlp&Version=2019-04-08&Action=ChatBot&SignVersion=
     * 不得不吐槽一下，加个密不至于这么复杂吧！！！
     */
    void chat(QString text, NetStringFunc func, int maxLen = 0)
    {
        if (secretId.isEmpty() || secretKey.isEmpty())
        {
            qWarning() << "腾讯智能闲聊：未设置ID或Key";
            return ;
        }
        if (text.isEmpty())
            return ;

        // 参数信息
        QString url = "https://nlp.tencentcloudapi.com/";
        MyJson queryJson;
        queryJson.insert("Query", text);

        // 公共参数
        qint64 timestamp = QDateTime::currentSecsSinceEpoch();
        QDateTime date = QDateTime::fromSecsSinceEpoch(timestamp);
        QByteArray timestampS = QString::number(timestamp).toUtf8();
        QByteArray dateS = date.toString("yyyy-MM-dd").toLocal8Bit();
        QByteArray service = "nlp";

        // 1. 拼接规范请求串
        QByteArray HTTPRequestMethod = "POST";
        QByteArray CanonicalURI  = "/";
        QByteArray CanonicalQueryString = "";
        QByteArray CanonicalHeaders = "content-type:application/json\nhost:nlp.tencentcloudapi.com\n";
        QByteArray SignedHeaders = "content-type;host";
        QByteArray content = queryJson.toBa(QJsonDocument::Compact);
        QByteArray HashedRequestPayload = QCryptographicHash::hash(content, QCryptographicHash::Sha256).toHex().toLower();
        QByteArray CanonicalRequest =
                                    HTTPRequestMethod + '\n' +
                                    CanonicalURI + '\n' +
                                    CanonicalQueryString + '\n' +
                                    CanonicalHeaders + '\n' +
                                    SignedHeaders + '\n' +
                                    HashedRequestPayload;

        // 2. 拼接待签名请求串
        QByteArray Algorithm = "TC3-HMAC-SHA256";
        QByteArray RequestTimestamp = timestampS;
        QByteArray CredentialScope = dateS + "/" + service + "/tc3_request";
        QByteArray HashedCanonicalRequest = QCryptographicHash::hash(CanonicalRequest, QCryptographicHash::Sha256).toHex().toLower();
        QByteArray StringToSign =
                                    Algorithm + '\n' +
                                    RequestTimestamp + '\n' +
                                    CredentialScope + '\n' +
                                    HashedCanonicalRequest;

        // 3. 计算签名
        QByteArray SecretId = secretId;
        QByteArray SecretKey = secretKey;
        qDebug() << secretId << secretKey;
        QByteArray SecretDate = QMessageAuthenticationCode::hash(dateS, "TC3" + SecretKey, QCryptographicHash::Sha256);
        QByteArray SecretService = QMessageAuthenticationCode::hash(service, SecretDate, QCryptographicHash::Sha256);
        QByteArray SecretSigning = QMessageAuthenticationCode::hash("tc3_request", SecretService, QCryptographicHash::Sha256);
        QByteArray Signature = QMessageAuthenticationCode::hash(StringToSign, SecretSigning, QCryptographicHash::Sha256).toHex();

        QByteArray auth = "TC3-HMAC-SHA256 Credential=" + SecretId + "/" + dateS + "/" + service + "/tc3_request, "
                "SignedHeaders=content-type;host, Signature=" + Signature;

        QNetworkAccessManager manager;
        QNetworkReply *reply;
        QNetworkRequest request(url);
        QEventLoop connectLoop;

        // 公共参数
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
        request.setRawHeader("X-TC-Action", "ChatBot");
        request.setRawHeader("X-TC-Region", "ap-guangzhou");
        request.setRawHeader("X-TC-Timestamp", timestampS);
        request.setRawHeader("X-TC-Version", "2019-04-08");
        request.setRawHeader("Authorization", auth);
        request.setRawHeader("Host", "nlp.tencentcloudapi.com");
        request.setRawHeader("X-TC-Language", "zh-CN");
        request.setRawHeader("X-TC-RequestClient", "APIExplorer");

        /* qDebug() << "------------post time             : " << timestamp;
        qDebug() << "------------post data             : " << queryJson.toBa(QJsonDocument::Compact);
        qDebug() << "------------HashedRequestPayload  : " << HashedRequestPayload;
        qDebug() << "------------CanonicalRequest      : " << CanonicalRequest;
        qDebug() << "------------StringToSign          : " << StringToSign;
        qDebug() << "------------Signature             : " << Signature;
        qDebug() << "------------authorization         : " << auth;
        foreach (auto h, request.rawHeaderList())
            qDebug() << "   " << h << request.rawHeader(h); */


        // 开始联网
        reply = manager.post(request, content);
        QObject::connect(reply, SIGNAL(finished()), &connectLoop, SLOT(quit()));
        connectLoop.exec();

        int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).value<int>();
        if (code != 200)
        {
            qWarning() << "腾讯智能闲聊 错误码：" << QString::number(code);
            return ;
        }
        if (reply->error() != QNetworkReply::NoError)
        {
            qWarning() << "腾讯智能闲聊 Error:" << reply->errorString();
            return ;
        }

        // 获取信息
        QByteArray result = reply->readAll();
        // qInfo() << "result:" << result;
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(result, &error);
        if (error.error != QJsonParseError::NoError)
        {
            qWarning() << "腾讯智能闲聊：" << error.errorString();
            return ;
        }

        QJsonObject json = document.object();
        if (!json.value("Response").toObject().value("Error").toObject().value("Code").toString().isEmpty())
        {
            QString msg = json.value("Response").toObject().value("Error").toObject().value("Message").toString();
            qWarning() << "AI回复：" << msg << queryJson.toBa(QJsonDocument::Compact);
            /* if (msg.contains("not found") && retry > 0)
                AIReply(id, text, func, maxLen, retry - 1); */
            return ;
        }

        MyJson resp = json.value("Response").toObject();
        QString answer = resp.s("Reply");
        // int confi = resp.d("Confidence"); // 置信度，0~1

        // 过滤文字
        if (answer.contains("未搜到")
                || answer.isEmpty())
            return ;

        if (maxLen > 0 && answer.length() > maxLen)
            return ;
        func(answer);
    }

private:
    QByteArray secretId;
    QByteArray secretKey;

    static TxNlp* txNlp;
};

#endif // TX_NLP_H
