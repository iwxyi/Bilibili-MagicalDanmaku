#include "voiceservice.h"

VoiceService::VoiceService(QObject *parent) : QObject(parent)
{

}

void VoiceService::speakText(QString text)
{
    // 处理特殊字符
    text.replace("_", " ");

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

void VoiceService::voiceDownloadAndSpeak(QString text)
{
    // TODO
}

void VoiceService::playNetAudio(QString url)
{
    // TODO
}
