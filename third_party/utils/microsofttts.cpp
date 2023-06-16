#include <QNetworkAccessManager>
#include <QNetworkReply>
#include "microsofttts.h"

MicrosoftTTS::MicrosoftTTS(QString dataPath, QString areaCode, QString key, QObject *parent)
    : QObject(parent), areaCode(areaCode), subscriptionKey(key)
{
    savedDir = dataPath + "tts/";
    QDir dir(savedDir);
    dir.mkpath(savedDir);

    fmt.setSampleRate(24000);  //设定播放采样频率为44100Hz的音频文件
    fmt.setSampleSize(16);     //设定播放采样格式（采样位数）为16位(bit)的音频文件。QAudioFormat支持的有8/16bit，即将声音振幅化为256/64k个等级
    fmt.setChannelCount(1);    //设定播放声道数目为2通道（立体声）的音频文件。mono(平声道)的声道数目是1，stero(立体声)的声道数目是2
    fmt.setCodec("audio/pcm"); //播放PCM数据（裸流）得设置编码器为"audio/pcm"。"audio/pcm"在所有的平台都支持，也就相当于音频格式的WAV,以线性方式无压缩的记录捕捉到的数据。如想使用其他编码格式 ，可以通过QAudioDeviceInfo::supportedCodecs()来获取当前平台支持的编码格式
    fmt.setByteOrder(QAudioFormat::LittleEndian); //设定字节序，以小端模式播放音频文件
    fmt.setSampleType(QAudioFormat::UnSignedInt); //设定采样类型。根据采样位数来设定。采样位数为8或16位则设置为QAudioFormat::UnSignedInt

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
    QString filePath = savedDir + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".mp3";
    QFile file(filePath);
    file.open(QFile::WriteOnly);
    QDataStream stream(&file);
    stream << ba;
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
        if (state == QAudio::IdleState)
        {
            audio->stop();
            audio->deleteLater();
            inputFile->close();
            if (deleteAfterPlay)
                inputFile->remove();
            inputFile->deleteLater();
            audio->deleteLater();
            audio = nullptr;

            // 播放下一个
            speakNext();
        }
    });
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

