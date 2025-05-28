#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "microsofttts.h"

MicrosoftTTS::MicrosoftTTS(QString dataPath, QString areaCode, QString key, QObject *parent)
    : QObject(parent), areaCode(areaCode), subscriptionKey(key)
{
    savedDir = dataPath + "tts/";
    QDir dir(savedDir);
    dir.mkpath(savedDir);

    fmt.setSampleRate(24000);  // 与微软TTS服务输出匹配
    fmt.setSampleSize(16);     // 16位采样
    fmt.setChannelCount(1);    // 单声道
    fmt.setCodec("audio/pcm"); // 使用PCM格式
    fmt.setByteOrder(QAudioFormat::LittleEndian);
    fmt.setSampleType(QAudioFormat::SignedInt);

    // 检查音频格式是否支持
    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());

    if (!info.isFormatSupported(fmt))
    {
        qWarning() << "MicrosoftTTS 不支持的音频格式，尝试使用最接近的格式";
        fmt = info.nearestFormat(fmt);
        qDebug() << "MicrosoftTTS 使用最接近的格式:";
        qDebug() << "  采样率:" << fmt.sampleRate();
        qDebug() << "  采样大小:" << fmt.sampleSize();
        qDebug() << "  声道数:" << fmt.channelCount();
        qDebug() << "  编码格式:" << fmt.codec();
        qDebug() << "  字节序:" << fmt.byteOrder();
        qDebug() << "  采样类型:" << fmt.sampleType();
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
    inputFile->open(QIODevice::ReadOnly);

    audio = new QAudioOutput(fmt);
    connect(audio, &QAudioOutput::stateChanged, this, [=](QAudio::State state) {
        qDebug() << "MicrosoftTTS 音频状态变化:" << state;
        if (state == QAudio::IdleState)
        {
            audio->stop();
            audio->deleteLater();
            audio = nullptr;
            inputFile->close();
            if (deleteAfterPlay)
                inputFile->remove();
            inputFile->deleteLater();

            // 播放下一个
            speakNext();
        }
        else if (state == QAudio::StoppedState)
        {
            if (audio->error() != QAudio::NoError)
            {
                qWarning() << "MicrosoftTTS 音频播放错误:" << audio->error();
                emit signalError("音频播放错误: " + QString::number(audio->error()));
            }
        }
    });

    // 检查音频格式是否支持
    QAudioDeviceInfo info(QAudioDeviceInfo::defaultOutputDevice());
    if (!info.isFormatSupported(fmt))
    {
        qWarning() << "MicrosoftTTS 不支持的音频格式，尝试使用最接近的格式";
        fmt = info.nearestFormat(fmt);
        qInfo() << "使用格式:" << fmt;
    }

    audio->start(inputFile);
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

