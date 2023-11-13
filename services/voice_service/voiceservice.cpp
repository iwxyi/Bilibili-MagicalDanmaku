#include "voiceservice.h"
#include "netutil.h"
#include "runtimeinfo.h"
#include "usersettings.h"

VoiceService::VoiceService(QObject *parent) : QObject(parent), NetInterface(this)
{

}

void VoiceService::initTTS()
{
    switch (voicePlatform) {
    case VoiceLocal:
#if defined(ENABLE_TEXTTOSPEECH)
        if (!tts)
        {
            qInfo() << "初始化TTS语音模块";
            tts = new QTextToSpeech(this);
            tts->setRate( (voiceSpeed = us->value("voice/speed", 50).toInt() - 50) / 50.0 );
            tts->setPitch( (voicePitch = us->value("voice/pitch", 50).toInt() - 50) / 50.0 );
            tts->setVolume( (voiceVolume = us->value("voice/volume", 50).toInt()) / 100.0 );
            connect(tts, &QTextToSpeech::stateChanged, this, [=](QTextToSpeech::State state){
                if (state == QTextToSpeech::Ready)
                    speakTextQueueNext();
            });
        }
#endif
        break;
    case VoiceXfy:
        if (!xfyTTS)
        {
            qInfo() << "初始化讯飞语音模块";
            xfyTTS = new XfyTTS(rt->dataPath,
                                us->value("xfytts/appid").toString(),
                                us->value("xfytts/apikey").toString(),
                                us->value("xfytts/apisecret").toString(),
                                this);
            xfyTTS->setName( voiceName = us->value("xfytts/name", "xiaoyan").toString() );
            xfyTTS->setPitch( voicePitch = us->value("voice/pitch", 50).toInt() );
            xfyTTS->setSpeed( voiceSpeed = us->value("voice/speed", 50).toInt() );
            xfyTTS->setVolume( voiceSpeed = us->value("voice/speed", 50).toInt() );
        }
        break;
    case VoiceMS:
        if (!msTTS)
        {
            qInfo() << "初始化微软语音模块";
            msTTS = new MicrosoftTTS(rt->dataPath,
                                     us->value("mstts/areaCode").toString(),
                                     us->value("mstts/subscriptionKey").toString(),
                                     this);
            connect(msTTS, &MicrosoftTTS::signalError, this, [=](QString err) {
                showError("微软语音", err);
            });
            msTTSFormat = us->value("mstts/format", DEFAULT_MS_TTS_SSML_FORMAT).toString();
            // TODO: 设置音调等内容
        }
        break;
    case VoiceCustom:
        break;
    }
}

void VoiceService::speakText(QString text)
{
    // 处理特殊字符
    text.replace("_", " ");
    text.replace("\\n", "\n");
    text.replace("%m%", "\n");
    text.replace("%n%", "\n");

    switch (voicePlatform) {
    case VoiceLocal:
#if defined(ENABLE_TEXTTOSPEECH)
        if (!tts)
            emit signalInit();
        else if (tts->state() != QTextToSpeech::Ready)
        {
            ttsQueue.append(text);
            return ;
        }
        tts->say(text);
#endif
        break;
    case VoiceXfy:
        if (!xfyTTS)
            emit signalInit();
        xfyTTS->speakText(text);
        break;
    case VoiceMS:
    {
        if (!msTTS)
            emit signalInit();

        // 替换文本
        if (!text.startsWith("<speak"))
        {
            QString rpl = msTTSFormat;
            text = rpl.replace("%text%", text);
        }
        msTTS->speakSSML(text);
    }
        break;
    case VoiceCustom:
        if (ttsDownloading || (ttsPlayer && ttsPlayer->state() == QMediaPlayer::State::PlayingState))
        {
            ttsQueue.append(text);
        }
        else
        {
            voiceDownloadAndSpeak(text);
        }
        break;
    }
}

void VoiceService::speakTextQueueNext()
{
    if (!ttsQueue.size())
        return ;
    QString text = ttsQueue.takeFirst();
    speakText(text);
}

/**
 * 针对返回是JSON/音频字节的类型
 * 自动判断
 */
void VoiceService::voiceDownloadAndSpeak(QString text)
{
    QString url = us->value("voice/customUrl", "").toString();
    if (url.isEmpty())
        return ;
    url = url.replace("%1", text).replace("%text%", text);
    url = url.replace("%url_text%", QString::fromUtf8(text.toUtf8().toPercentEncoding()));
    playNetAudio(url);
}

void VoiceService::playNetAudio(QString url)
{
    qInfo() << "播放网络音频：" << url;
    const QString filePath = rt->dataPath + "tts";
    QDir dir(filePath);
    dir.mkpath(filePath);

    ttsDownloading = true;
    get(url, [=](QNetworkReply* reply1){
        ttsDownloading = false;
        QByteArray fileData;

        if (reply1->error() != QNetworkReply::NoError)
        {
            qWarning() << "获取网络音频错误：" << reply1->errorString();
            showError("获取音频错误", reply1->errorString());
            return ;
        }

        auto contentType = reply1->header(QNetworkRequest::ContentTypeHeader).toString();
        if (contentType.contains("json"))
        {
            // https://ai.baidu.com/aidemo?type=tns&per=4119&spd=6&pit=5&vol=5&aue=3&tex=这是一个测试文本
            QByteArray ba = reply1->readAll();
            MyJson json(ba);
            QString s = json.s("data");
            if (s.startsWith("data:audio") && s.contains(","))
            {
                QStringList sl = s.split(",");
                fileData = sl.at(1).toUtf8();
                if (sl.at(0).contains("base64"))
                    fileData = QByteArray::fromBase64(fileData);
            }
            else
            {
                qWarning() << "无法解析的语音返回：" << json;
                showError("在线音频", json.errOrMsg());
                return ;
            }
        }
        else
        {
            // http://120.24.87.124/cgi-bin/ekho2.pl?cmd=SPEAK&voice=EkhoMandarin&speedDelta=0&pitchDelta=0&volumeDelta=0&text=这是一个测试文本
            fileData = reply1->readAll();
        }

        if (fileData.isEmpty())
        {
            qWarning() << "网络音频为空";
            return ;
        }

        // 保存文件
        QString path = filePath + "/" + QString::number(QDateTime::currentMSecsSinceEpoch()) + ".mp3";
        QFile file(path);
        file.open(QFile::WriteOnly);
        QDataStream stream(&file);
        stream << fileData;
        file.flush();
        file.close();

        // 播放文件
        if (!ttsPlayer)
        {
            ttsPlayer = new QMediaPlayer(this);
            connect(ttsPlayer, &QMediaPlayer::stateChanged, this, [=](QMediaPlayer::State state){
                if (state == QMediaPlayer::StoppedState)
                {
                    QFile file(path);
                    file.remove();
                    speakTextQueueNext();
                }
            });
        }
        ttsPlayer->setMedia(QUrl::fromLocalFile(path));

        ttsPlayer->play();
    });
}

void VoiceService::showError(QString title, QString msg)
{
    emit signalError(title, msg);
}
