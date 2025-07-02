#include <QApplication>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonDocument>
#include <QJsonParseError>
#include <QAudioSink>
#include "xfytts.h"

XfyTTS::XfyTTS(QString dataPath, QString APPID, QString APIKey, QString APISecret, QObject *parent)
    : QObject(parent), APPID(APPID), APIKey(APIKey), APISecret(APISecret)
{
    AUTH_DEB << APIKey << APISecret;
    savedDir = dataPath + "tts/";
    QDir dir(savedDir);
    dir.mkpath(savedDir);

    fmt.setSampleRate(16000);       // 采样率 16kHz（常用语音采样率）
    fmt.setChannelCount(1);         // 单声道（语音通常用单声道）

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Qt5 专用设置
    fmt.setSampleSize(16);          // 采样位数 16-bit
    fmt.setCodec("audio/pcm");      // PCM编码（裸流）
    fmt.setByteOrder(QAudioFormat::LittleEndian); // 小端字节序
    fmt.setSampleType(QAudioFormat::UnSignedInt); // 无符号整型采样
#else
    // Qt6 专用设置
    fmt.setSampleFormat(QAudioFormat::Int16);     // 16位有符号整型（替代Qt5的setSampleSize+setSampleType）
    // 注意：Qt6中不再需要单独设置：
    // - codec（自动根据sampleFormat确定）
    // - byteOrder（自动处理）
#endif
}

void XfyTTS::speakText(QString text)
{
    speakQueue.append(text);
    if (socket) // 表示正在连接
        return ;

    startConnect();
}

void XfyTTS::speakNext()
{
    if (socket || !speakQueue.size()) // 表示正在连接
        return ;

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

    playing = true;
    socket = new QWebSocket();
    QByteArray* receivedBytes = new QByteArray;
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
    });
    connect(socket, &QWebSocket::stateChanged, this, [=](QAbstractSocket::SocketState state){
        AUTH_DEB << "tts stateChanged" << state;
    });
    connect(socket, &QWebSocket::binaryMessageReceived, this, [=](const QByteArray &message){
        AUTH_DEB << "tts binaryMessageReceived" << message;
    });
    connect(socket, &QWebSocket::textFrameReceived, this, [=](const QString &frame, bool isLastFrame){
        AUTH_DEB << "textFrameReceived" << frame.size() << isLastFrame;
        QJsonParseError error;
        QJsonDocument document = QJsonDocument::fromJson(frame.toUtf8(), &error);
        if (error.error != QJsonParseError::NoError)
        {
            qWarning() << "解析语音合成结果出错:" << error.errorString();
            return ;
        }
        QJsonObject json = document.object();
        if (json.value("code").toInt() != 0)
        {
            qWarning() << "TextMessage 返回 code 不为0：" << frame.left(1000);
            return ;
        }
        QJsonObject data= json.value("data").toObject();
        int status = data.value("status").toInt();
        QString audio = data.value("audio").toString();

        QByteArray ba = QByteArray::fromBase64(audio.toUtf8());
//        qDebug() << "~~~~~~~~~~" << ba.length() << ba.left(200).toHex();
        receivedBytes->append(ba);

        if (status == 2) // 结束了
        {
            generalAudio(receivedBytes);
            socket->close();
        }
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
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    config.setProtocol(QSsl::TlsV1SslV3);
#else
    config.setProtocol(QSsl::SecureProtocols);  // 使用安全协议集合
#endif    socket->setSslConfiguration(config);

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
                            "    \"auf\": \"audio/L16;rate=16000\","
                            "    \"vcn\": \"%3\","
                            "    \"tte\": \"utf8\","
                            "    \"pitch\": %4,"
                            "    \"speed\": %5"
                            "  },"
                            "  \"data\": {"
                            "    \"status\": 2,"
                            "    \"text\": \"%6\""
                            "  }"
                            "}").arg(APPID).arg("raw").arg(vcn).arg(pitch).arg(speed)
            .arg(QString::fromUtf8(text.toUtf8().toBase64()));
    AUTH_DEB << param;
    socket->sendTextMessage(param);
}

void XfyTTS::generalAudio(QByteArray* ba)
{
    QString filePath = savedDir + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".pcm";
    AUTH_DEB << "saveFile:" << filePath;

    QFile file(filePath);
    file.open(QFile::WriteOnly);
    QDataStream stream(&file);
    stream << *ba;
    ba->clear();
    delete ba;
    file.flush();
    file.close();

    emit signalTTSPrepared(filePath);
    playFile(filePath, true);
}

void XfyTTS::playFile(QString filePath, bool deleteAfterPlay)
{
    QFile *inputFile = new QFile(filePath);
    inputFile->open(QIODevice::ReadOnly);

    // Qt5/Qt6兼容处理
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    QAudioSink *audio = new QAudioSink(fmt, this);
#else
    QAudioOutput *audio = new QAudioOutput(fmt, this);
#endif

    // Qt5/Qt6状态枚举兼容
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    using AudioState = QAudio::State;
#else
    using AudioState = QAudio::State;
#endif

    // 连接状态变化信号
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    connect(audio, &QAudioSink::stateChanged, this, [=](AudioState state) {
#else
    connect(audio, &QAudioOutput::stateChanged, this, [=](AudioState state) {
#endif
        if (state == QAudio::IdleState)
        {
            audio->stop();
            audio->deleteLater();
            inputFile->close();
            if (deleteAfterPlay)
                inputFile->remove();
            inputFile->deleteLater();

            speakNext();
        }
        playing = false;
    });

    // Qt5/Qt6启动方式兼容
#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    audio->start(inputFile);
#else
    audio->start(inputFile);
#endif
}
void XfyTTS::setAppId(QString s)
{
    this->APPID = s.trimmed();
}

void XfyTTS::setApiKey(QString s)
{
    this->APIKey = s.trimmed();
}

void XfyTTS::setApiSecret(QString s)
{
    this->APISecret = s.trimmed();
}

void XfyTTS::setName(QString name)
{
    this->vcn = name;
}

void XfyTTS::setPitch(int pitch)
{
    this->pitch = pitch;
}

void XfyTTS::setSpeed(int speed)
{
    this->speed = speed;
}

void XfyTTS::setVolume(int volume)
{
    this->volume = volume;
}

bool XfyTTS::isPlaying() const
{
    return playing;
}
