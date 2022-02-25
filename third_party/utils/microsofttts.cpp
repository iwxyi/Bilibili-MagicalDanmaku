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

    QTimer* refreshTimer = new QTimer(this);
    refreshTimer->setInterval(9 * 60 * 1000); // 9分钟
    connect(refreshTimer, &QTimer::timeout, this, &MicrosoftTTS::refreshToken);
    refreshTimer->start();

    refreshToken();
}

void MicrosoftTTS::speakText(QString ssml)
{
    speakQueue.append(ssml);
    if (audio || getting)
        return ;

    speakNext();
}

void MicrosoftTTS::speakNext()
{
    if (speakQueue.isEmpty())
        return ;

    QString ssml = speakQueue.takeFirst();
    if (accessToken.isEmpty())
    {
        qWarning() << "MicrosoftTTS not get AccessToken";
        return ;
    }
    getting = true;

    // 获取数据
    QUrl url("https://" + areaCode + ".tts.speech.microsoft.com/cognitiveservices/v1");
    QNetworkAccessManager manager;
    QEventLoop loop;
    QNetworkReply *reply;
    QNetworkRequest request(url);

    request.setRawHeader("Authorization", "Bearer " + accessToken);
    request.setRawHeader("Content-Type", "application/ssml+xml");
    request.setRawHeader("X-Microsoft-OutputFormat", "raw-24khz-16bit-mono-pcm");
    request.setRawHeader("User-Agent", "MagicalDanmaku");

    reply = manager.post(request, ssml.toUtf8());
    QObject::connect(reply, SIGNAL(finished()), &loop, SLOT(quit())); //请求结束并下载完成后，退出子事件循环
    loop.exec(); //开启子事件循环

    // 判断结果
    int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).value<int>();
    if (code != 200)
    {
        qWarning() << "MicrosoftTTS Error Code:" << code;
    }
    if (reply->error() != QNetworkReply::NoError)
    {
        qWarning() << "MicrosoftTTS TTS Error:" << reply->errorString();
    }

    QByteArray ba = reply->readAll();
    reply->deleteLater();

    // 保存文件
    QString filePath = savedDir + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".mp3";
    QFile file(filePath);
    file.open(QFile::WriteOnly);
    QDataStream stream(&file);
    stream << ba;
    file.flush();
    file.close();
    qDebug() << "MicrosoftTTS 保存文件：" << filePath;

    // 播放音频
    playFile(filePath, false);

    getting = false;
}

void MicrosoftTTS::playFile(QString filePath, bool deleteAfterPlay)
{
    QFile *inputFile = new QFile(filePath);
    inputFile->open(QIODevice::ReadOnly);

    QAudioOutput *audio = new QAudioOutput(fmt);
    connect(audio, &QAudioOutput::stateChanged, this, [=](QAudio::State state) {
        if (state == QAudio::IdleState)
        {
            audio->stop();
            audio->deleteLater();
            inputFile->close();
            if (deleteAfterPlay)
                inputFile->remove();
            inputFile->deleteLater();

            // 播放下一个
            speakNext();
        }
    });
    audio->start(inputFile);
}

void MicrosoftTTS::setSubscriptionKey(QString key)
{
    this->subscriptionKey = key;
    refreshToken();
}

void MicrosoftTTS::refreshToken()
{
    if (areaCode.isEmpty() || subscriptionKey.isEmpty())
    {
        qWarning() << "MicrosoftTTS Unset HostUrl or SubscriptionKey";
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
    if (accessToken.isEmpty())
        qWarning() << "MicrosoftTTS can't get AccessToken:" << url;
    else
        qInfo() << "MicrosoftTTS access token" << accessToken;
}
