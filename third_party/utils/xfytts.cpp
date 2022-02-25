#include <QApplication>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonDocument>
#include <QJsonParseError>
#include "xfytts.h"

XfyTTS::XfyTTS(QString dataPath, QString APPID, QString APIKey, QString APISecret, QObject *parent)
    : QObject(parent), APPID(APPID), APIKey(APIKey), APISecret(APISecret)
{
    AUTH_DEB << APIKey << APISecret;
    savedDir = dataPath + "tts/";
    QDir dir(savedDir);
    dir.mkpath(savedDir);

    fmt.setSampleRate(16000);  //设定播放采样频率为44100Hz的音频文件
    fmt.setSampleSize(16);     //设定播放采样格式（采样位数）为16位(bit)的音频文件。QAudioFormat支持的有8/16bit，即将声音振幅化为256/64k个等级
    fmt.setChannelCount(1);    //设定播放声道数目为2通道（立体声）的音频文件。mono(平声道)的声道数目是1，stero(立体声)的声道数目是2
    fmt.setCodec("audio/pcm"); //播放PCM数据（裸流）得设置编码器为"audio/pcm"。"audio/pcm"在所有的平台都支持，也就相当于音频格式的WAV,以线性方式无压缩的记录捕捉到的数据。如想使用其他编码格式 ，可以通过QAudioDeviceInfo::supportedCodecs()来获取当前平台支持的编码格式
    fmt.setByteOrder(QAudioFormat::LittleEndian); //设定字节序，以小端模式播放音频文件
    fmt.setSampleType(QAudioFormat::UnSignedInt); //设定采样类型。根据采样位数来设定。采样位数为8或16位则设置为QAudioFormat::UnSignedInt
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
    config.setProtocol(QSsl::TlsV1SslV3);
    socket->setSslConfiguration(config);

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

    QAudioOutput *audio = new QAudioOutput(fmt);
//    audio->setVolume(volume / 100.0); // 设置音量没有效果，1正常，0静音，但是0.x会呲呲呲的响
    connect(audio, &QAudioOutput::stateChanged, this, [=](QAudio::State state) {
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
    });
    audio->start(inputFile);
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
