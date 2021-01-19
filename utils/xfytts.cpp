#include <QApplication>
#include "xfytts.h"

XfyTTS::XfyTTS(QString APPID, QString APIKey, QString APISecret, QObject *parent)
    : QObject(parent), APPID(APPID), APIKey(APIKey), APISecret(APISecret)
{
    AUTH_DEB << APIKey << APISecret;
    savedDir = QApplication::applicationDirPath() + "/audios/";
    QDir dir(savedDir);
    dir.mkpath(savedDir);
}

void XfyTTS::speakText(QString text)
{
    if (socket) // 表示正在连接
        return ;

    speakQueue.append(text);
    startConnect();
}

void XfyTTS::startConnect()
{
    if (APIKey.isEmpty() || APISecret.isEmpty())
    {
        qCritical() << "请输入APIKey 和 APISecret";
        return ;
    }
    if (socket)
        return ;

    socket = new QWebSocket();
    connect(socket, &QWebSocket::connected, this, [=]{
        AUTH_DEB << "tts connected";
        if (!speakQueue.size())
        {
            socket->close();
            return ;
        }

        sendText(speakQueue.takeFirst());
    });
    connect(socket, &QWebSocket::disconnected, this, [=]{
        AUTH_DEB << "tts disconnected" << socket->closeCode() << socket->closeReason();
        socket->deleteLater();
        socket = nullptr;
        generalAudio();
    });
    connect(socket, &QWebSocket::stateChanged, this, [=](QAbstractSocket::SocketState state){
        AUTH_DEB << "tts stateChanged" << state;
    });
    connect(socket, &QWebSocket::binaryMessageReceived, this, [=](const QByteArray &message){
        AUTH_DEB << "tts binaryMessageReceived" << message;
    });
    connect(socket, &QWebSocket::textFrameReceived, this, [=](const QString &frame, bool isLastFrame){
        AUTH_DEB << "textFrameReceived" << frame.size() << isLastFrame;
        receivedData += frame;
    });

    connect(socket, &QWebSocket::textMessageReceived, this, [=](const QString &message){
        AUTH_DEB << "textMessageReceived" << message.size();
        // 实测讯飞返回的数据，这里textFrame和textMessage都会收到，数据一样，并且isLastFrame都是true
    });

    // 开始连接
    QString url = hostUrl + "?authorization=" + getAuthorization().toLocal8Bit().toPercentEncoding()
            + "&date=" + getDate().toLocal8Bit().toPercentEncoding().replace("%20", "+")
            + "&host=ws-api.xfyun.cn";
    AUTH_DEB << "wss:" << url;

    // 设置安全套接字连接模式（不知道有啥用）
    QSslConfiguration config = socket->sslConfiguration();
    config.setPeerVerifyMode(QSslSocket::VerifyNone);
    config.setProtocol(QSsl::TlsV1SslV3);
    socket->setSslConfiguration(config);

    receivedData.clear();
    socket->open(url);
}

QString XfyTTS::getAuthorization() const
{
    QString auth = QString("api_key=\"%1\", algorithm=\"hmac-sha256\", headers=\"host date request-line\", signature=\"%2\"")
            .arg(APIKey).arg(getSignature());
    AUTH_DEB << "authorization:" << auth;
    AUTH_DEB << "authorization.base64:" << auth.toLocal8Bit().toBase64();
    return auth.toLocal8Bit().toBase64();
}

QString XfyTTS::getSignature() const
{
    QString sign = QString("host: ws-api.xfyun.cn\ndate: %1\nGET /v2/tts HTTP/1.1").arg(getDate());
    AUTH_DEB << "signature:" << sign;
    QByteArray sign_sha256 = QMessageAuthenticationCode::hash(sign.toLocal8Bit(), APISecret.toLocal8Bit(), QCryptographicHash::Sha256);
    AUTH_DEB << "signature.sha256:" << sign_sha256.toHex();
    AUTH_DEB << "signature.sha256.base64:" << sign_sha256.toBase64();
    return sign_sha256.toBase64();
//    return toHmacSha1Base64(APISecret.toLocal8Bit(), sign.toLocal8Bit()); // 不带中文的话 toUTF8() 也行
}

/// https://blog.csdn.net/itas109/article/details/79362293
QString XfyTTS::getDate() const
{
//    return "Tue, 19 Jan 2021 07:07:28 GMT";
    QLocale locale = QLocale::English;
    QString localeTime = locale.toString(QDateTime::currentDateTime().toUTC(),"ddd, dd MMM yyyy hh:mm:ss");
    return localeTime + " GMT";
}

void XfyTTS::sendText(QString text)
{
    QString param = QString("{"
                            "  \"common\": {"
                            "    \"app_id\": \"%1\""
                            "  },"
                            "  \"business\": {"
                            "    \"aue\": \"%2\","
                            "    \"vcn\": \"%3\","
                            "    \"pitch\": %4,"
                            "    \"speed\": %5"
                            "  },"
                            "  \"data\": {"
                            "    \"status\": 2,"
                            "    \"text\": \"%6\""
                            "  }"
                            "}").arg(APPID).arg("raw").arg("xiaoyan").arg(pitch).arg(speed)
            .arg(QString::fromLocal8Bit(text.toLocal8Bit().toBase64()));
    AUTH_DEB << param;
    socket->sendTextMessage(param);
}

void XfyTTS::generalAudio()
{
    QString filePath = savedDir + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".pcm";
    AUTH_DEB << "saveFile:" << filePath;

    QFile file(filePath);
    file.open(QFile::WriteOnly | QFile::Text);
    QTextStream stream(&file);
    stream << receivedData;
    file.close();

    emit signalTTSPrepared(filePath);
}
