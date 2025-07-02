#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "microsofttts.h"

MicrosoftTTS::MicrosoftTTS(QString dataPath, QString areaCode, QString key, QObject *parent)
    : QObject(parent), areaCode(areaCode), subscriptionKey(key)
{
    savedDir = dataPath + "tts/";
    QDir dir(savedDir);
    dir.mkpath(savedDir);

    {
        // 设置音频格式
        QAudioFormat fmt;
        fmt.setSampleRate(24000);  // 采样率
        fmt.setChannelCount(1);    // 单声道

        // Qt5/Qt6 兼容的格式设置
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        // Qt5 设置方式
        fmt.setSampleSize(16);
        fmt.setByteOrder(QAudioFormat::LittleEndian);
        fmt.setSampleType(QAudioFormat::SignedInt);
        fmt.setCodec("audio/pcm");
#else
        // Qt6 设置方式
        fmt.setSampleFormat(QAudioFormat::Int16);
#endif

        // Qt5/Qt6 兼容的设备检查
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
        bool isSupported = info.isFormatSupported(fmt);
        if (!isSupported) {
            qWarning() << "不支持的音频格式，尝试使用最接近的格式";
            fmt = info.nearestFormat(fmt);
        }
#else
        QAudioDevice info = QMediaDevices::defaultAudioOutput();
        bool isSupported = info.isFormatSupported(fmt);
        if (!isSupported) {
            qWarning() << "Qt6不再提供nearestFormat，请手动调整格式参数";
            // Qt6下需要手动调整格式，例如尝试其他采样格式
            fmt.setSampleFormat(QAudioFormat::Float);  // 尝试浮点格式
            isSupported = info.isFormatSupported(fmt);
        }
#endif

        // 输出最终格式信息
        qDebug() << "最终使用的音频格式:";
        qDebug() << "  采样率:" << fmt.sampleRate();
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        qDebug() << "  采样大小:" << fmt.sampleSize();
        qDebug() << "  编码格式:" << fmt.codec();
        qDebug() << "  字节序:" << fmt.byteOrder();
        qDebug() << "  采样类型:" << fmt.sampleType();
#else
        qDebug() << "  采样格式:" << fmt.sampleFormat();
#endif
        qDebug() << "  声道数:" << fmt.channelCount();
    }

    refreshTimer = new QTimer(this);
    refreshTimer->setInterval(9 * 60 * 1000); // 9分钟
    connect(refreshTimer, &QTimer::timeout, this, &MicrosoftTTS::refreshToken);
    refreshTimer->start();

    timeoutTimer = new QTimer(this);
    timeoutTimer->setInterval(30 * 1000); // MS语音合成+下载的超时时间
    timeoutTimer->setSingleShot(true);
    connect(timeoutTimer, &QTimer::timeout, this, [=]{
        qWarning() << "MicroSoftTTS 超时";
        connectLoop.quit();
    });

    refreshToken();
}

void MicrosoftTTS::speakSSML(QString ssml)
{
    speakQueue.append(ssml);
    if (isDownloadingOrSpeaking())
    {
        qInfo() << "播放 SSML 进入队列，当前数量：" << speakQueue.size();
        return ;
    }

    speakNext();
}

void MicrosoftTTS::speakNext()
{
    playing = false;
    if (speakQueue.isEmpty())
        return ;
    if (accessToken.isEmpty())
    {
        qWarning() << "MicrosoftTTS not get AccessToken";
        return ;
    }

    playing = true;
    QString ssml = speakQueue.takeFirst();
    // getting = true;

    // 获取数据
    QUrl url("https://" + areaCode + ".tts.speech.microsoft.com/cognitiveservices/v1");
    QNetworkAccessManager manager;
    QNetworkReply *reply;
    QNetworkRequest request(url);

    request.setRawHeader("Authorization", "Bearer " + accessToken);
    request.setRawHeader("Content-Type", "application/ssml+xml");
    request.setRawHeader("X-Microsoft-OutputFormat", "raw-24khz-16bit-mono-pcm");
    request.setRawHeader("User-Agent", "MagicalDanmaku");

    reply = manager.post(request, ssml.toUtf8());
    QObject::connect(reply, SIGNAL(finished()), &connectLoop, SLOT(quit())); //请求结束并下载完成后，退出子事件循环
    timeoutTimer->start();
    connectLoop.exec(); //开启子事件循环

    // 结束循环，可能是超时
    // getting = false;
    if (!timeoutTimer->isActive())
    {
        qWarning() << "超时，停止语音，播放下一个";
        reply->abort();
        reply->deleteLater();
        speakNext();
        return ;
    }
    timeoutTimer->stop();

    // 判断结果
    int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).value<int>();
    if (code != 200)
    {
        qWarning() << "MicrosoftTTS Error Code:" << code;
        emit signalError("错误码：" + QString::number(code));
    }
    if (reply->error() != QNetworkReply::NoError)
    {
        qWarning() << "MicrosoftTTS TTS Error:" << reply->errorString();
        emit signalError(reply->errorString());
    }

    QByteArray ba = reply->readAll();
    reply->deleteLater();
    if (ba.isEmpty())
    {
        qWarning() << "MicrosoftTTS 空音频文件";
        speakNext();
        return ;
    }

    // 保存文件
    QString filePath = savedDir + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".wav";
    QFile file(filePath);
    file.open(QFile::WriteOnly);
    
    // 写入WAV文件头
    // RIFF header
    file.write("RIFF", 4);
    // 文件大小（不包括RIFF和大小字段）
    quint32 fileSize = ba.size() + 36;
    file.write(reinterpret_cast<char*>(&fileSize), 4);
    file.write("WAVE", 4);
    
    // fmt chunk
    file.write("fmt ", 4);
    // fmt chunk size
    quint32 fmtSize = 16;
    file.write(reinterpret_cast<char*>(&fmtSize), 4);
    // 音频格式（1表示PCM）
    quint16 audioFormat = 1;
    file.write(reinterpret_cast<char*>(&audioFormat), 2);
    // 声道数
    quint16 numChannels = 1;
    file.write(reinterpret_cast<char*>(&numChannels), 2);
    // 采样率
    quint32 sampleRate = 24000;
    file.write(reinterpret_cast<char*>(&sampleRate), 4);
    // 字节率
    quint32 byteRate = sampleRate * numChannels * 2;
    file.write(reinterpret_cast<char*>(&byteRate), 4);
    // 块对齐
    quint16 blockAlign = numChannels * 2;
    file.write(reinterpret_cast<char*>(&blockAlign), 2);
    // 位深度
    quint16 bitsPerSample = 16;
    file.write(reinterpret_cast<char*>(&bitsPerSample), 2);
    
    // data chunk
    file.write("data", 4);
    // data chunk size
    quint32 dataSize = ba.size();
    file.write(reinterpret_cast<char*>(&dataSize), 4);
    
    // 写入PCM数据
    file.write(ba);
    file.flush();
    file.close();
    qInfo() << "MicrosoftTTS 保存文件：" << ba.size() << filePath;

    // 播放音频
    playFile(filePath, true);
}

void MicrosoftTTS::playFile(QString filePath, bool deleteAfterPlay)
{
    playing = true;
    QFile *inputFile = new QFile(filePath);
    if (!inputFile->open(QIODevice::ReadOnly)) {
        qWarning() << "无法打开音频文件:" << filePath;
        inputFile->deleteLater();
        return;
    }

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    // Qt5 实现
    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(fmt)) {
        fmt = info.nearestFormat(fmt);
    }
    audioOutput = new QAudioOutput(fmt, this);

    connect(audioOutput, &QAudioOutput::stateChanged, this, [=](QAudio::State state) {
        handleAudioState(state, inputFile, deleteAfterPlay);
    });
#else
    // Qt6 实现
    QAudioDevice device = QMediaDevices::defaultAudioOutput();
    audioSink = new QAudioSink(device, fmt, this);

    connect(audioSink, &QAudioSink::stateChanged, this, [=]() {
        handleAudioState(audioSink->state(), inputFile, deleteAfterPlay);
    });
#endif

// 开始播放
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    audioOutput->start(inputFile);
#else
    audioSink->start(inputFile);
#endif
}

void MicrosoftTTS::handleAudioState(QAudio::State state, QFile* file, bool deleteAfterPlay)
{
    if (state == QAudio::IdleState || state == QAudio::StoppedState) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        audioOutput->stop();
        audioOutput->deleteLater();
#else
        audioSink->stop();
        audioSink->deleteLater();
#endif

        file->close();
        if (deleteAfterPlay) file->remove();
        file->deleteLater();

        QMetaObject::invokeMethod(this, &MicrosoftTTS::speakNext, Qt::QueuedConnection);
    }
}

void MicrosoftTTS::setAreaCode(QString area)
{
    this->areaCode = area;
    refreshToken();
}

void MicrosoftTTS::setSubscriptionKey(QString key)
{
    this->subscriptionKey = key;
    refreshToken();
}

bool MicrosoftTTS::isPlaying() const
{

}

void MicrosoftTTS::refreshToken()
{
    if (areaCode.isEmpty() || subscriptionKey.isEmpty())
    {
        qWarning() << "MicrosoftTTS Unset HostUrl or SubscriptionKey";
        return ;
    }

    if (subscriptionKey == "试听")
    {
        // 有效时间5分钟，每4分半获取一次
        refreshTimer->setInterval(270*1000);
        getWhitePiaoToken();
        return ;
    }

    QUrl url("https://" + areaCode + ".api.cognitive.microsoft.com/sts/v1.0/issuetoken");
    QNetworkAccessManager manager;
    QEventLoop loop;
    QNetworkReply *reply;
    QNetworkRequest request(url);

    // 加上这个才能获取令牌
    request.setRawHeader("Ocp-Apim-Subscription-Key", subscriptionKey.toLatin1());

    reply = manager.post(request, QByteArray());
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit())); //请求结束并下载完成后，退出子事件循环
    loop.exec(); //开启子事件循环

    accessToken = reply->readAll();
    reply->deleteLater();
    if (!accessToken.isEmpty())
    {
        qInfo() << "MicrosoftTTS access token" << accessToken;
        // 初始化结束后需要播放语音
        if (!isDownloadingOrSpeaking())
            speakNext();
    }
    else
    {
        qWarning() << "MicrosoftTTS can't get AccessToken:" << url;
        emit signalError("无法获取 AccessToken");
    }

}

void MicrosoftTTS::getWhitePiaoToken()
{
    QUrl url("https://azure.microsoft.com/zh-cn/services/cognitive-services/text-to-speech/");
    QNetworkAccessManager manager;
    QEventLoop loop;
    QNetworkReply *reply;
    QNetworkRequest request(url);

    reply = manager.get(request);
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit())); //请求结束并下载完成后，退出子事件循环
    loop.exec(); //开启子事件循环

    accessToken.clear();
    QString content = reply->readAll();
    if (content.isEmpty())
        qWarning() << "MicrosoftTTS can't get TextToSpeech HTML";
    int start = content.indexOf("token: \"");
    if (start > -1)
    {
        start += 8;
        int end = content.indexOf("\"", start);
        if (end > -1)
        {
            accessToken = content.mid(start, end - start).toLatin1();
            qInfo() << "get free AccessToken：" << accessToken;
        }
    }
    reply->deleteLater();
}

void MicrosoftTTS::clearQueue()
{
    speakQueue.clear();
    // getting = false;
}

bool MicrosoftTTS::isDownloadingOrSpeaking() const
{
    return timeoutTimer->isActive() || audio;
}

