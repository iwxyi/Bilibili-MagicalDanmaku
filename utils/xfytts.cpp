#include <QOAuth1Signature>
#include <QApplication>
#include <QClipboard>
#include "xfytts.h"

XfyTTS::XfyTTS(QString APIKey, QString APISecret, QObject *parent)
    : QObject(parent), APIKey(APIKey), APISecret(APISecret)
{
    AUTH_DEB << APIKey << APISecret;
}

void XfyTTS::startConnect()
{
    if (APIKey.isEmpty() || APISecret.isEmpty())
    {
        qDebug() << "请输入APIKey 和 APISecret";
        return ;
    }

    if (!socket)
    {
        socket = new QWebSocket();
        connect(socket, &QWebSocket::connected, this, [=]{
            qDebug() << "tts connected";
        });
        connect(socket, &QWebSocket::disconnected, this, [=]{
            qDebug() << "tts disconnected" << socket->closeCode() << socket->closeReason();
        });
        connect(socket, &QWebSocket::stateChanged, this, [=](QAbstractSocket::SocketState state){
            qDebug() << "tts stateChanged" << state;
        });
        connect(socket, &QWebSocket::binaryMessageReceived, this, [=](const QByteArray &message){
            qDebug() << "tts binaryMessageReceived" << message;
        });
        connect(socket, &QWebSocket::textFrameReceived, this, [=](const QString &frame, bool isLastFrame){
            qDebug() << "textFrameReceived";
        });

        connect(socket, &QWebSocket::textMessageReceived, this, [=](const QString &message){
            qDebug() << "textMessageReceived";
        });
    }

    QString url = hostUrl + "?authorization=" + getAuthorization().toLocal8Bit().toPercentEncoding()
            + "&date=" + getDate().toLocal8Bit().toPercentEncoding().replace("%20", "+")
            + "&host=ws-api.xfyun.cn";
    AUTH_DEB << "wss:" << url;

    // 设置安全套接字连接模式（不知道有啥用）
    QSslConfiguration config = socket->sslConfiguration();
    config.setPeerVerifyMode(QSslSocket::VerifyNone);
    config.setProtocol(QSsl::TlsV1SslV3);
    socket->setSslConfiguration(config);

    QNetworkRequest* request = new QNetworkRequest(url);

    socket->open(*request);

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

/// https://blog.csdn.net/qiangzi4646/article/details/73565070
QString XfyTTS::toHmacSha1Base64(QByteArray key, QByteArray baseString) const
{
    int blockSize = 64; // HMAC-SHA-1 block size, defined in SHA-1 standard
    if (key.length() > blockSize) { // if key is longer than block size (64), reduce key length with SHA-1 compression
        key = QCryptographicHash::hash(key, QCryptographicHash::Sha1);
    }
    QByteArray innerPadding(blockSize, char(0x36)); // initialize inner padding with char"6"
    QByteArray outerPadding(blockSize, char(0x5c)); // initialize outer padding with char"/"
    // ascii characters 0x36 ("6") and 0x5c ("/") are selected because they have large
    // Hamming distance (http://en.wikipedia.org/wiki/Hamming_distance)
    for (int i = 0; i < key.length(); i++) {
        innerPadding[i] = innerPadding[i] ^ key.at(i); // XOR operation between every byte in key and innerpadding, of key length
        outerPadding[i] = outerPadding[i] ^ key.at(i); // XOR operation between every byte in key and outerpadding, of key length
    }
    // result = hash ( outerPadding CONCAT hash ( innerPadding CONCAT baseString ) ).toBase64
    QByteArray total = outerPadding;
    QByteArray part = innerPadding;
    part.append(baseString);
    total.append(QCryptographicHash::hash(part, QCryptographicHash::Sha1));
    QByteArray hashed = QCryptographicHash::hash(total, QCryptographicHash::Sha1);
    AUTH_DEB << "signature.sha1:" << hashed.toHex();
    AUTH_DEB << "signature.sha1.base64:" << hashed.toBase64();
    return hashed.toBase64();
}
