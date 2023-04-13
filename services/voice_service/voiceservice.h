#ifndef VOICESERVICE_H
#define VOICESERVICE_H

#include <QObject>
#if defined(ENABLE_TEXTTOSPEECH)
#include <QtTextToSpeech/QTextToSpeech>
#endif
#include <QMediaPlayer>
#include "xfytts.h"
#include "microsofttts.h"

enum VoicePlatform
{
    VoiceLocal,
    VoiceXfy,
    VoiceCustom,
    VoiceMS,
};

class VoiceService : public QObject
{
    Q_OBJECT
public:
    explicit VoiceService(QObject *parent = nullptr);


signals:
    void signalInit();
    void signalInitFinished();

public slots:
    void speakText(QString text);
    void speakTextQueueNext();
    void voiceDownloadAndSpeak(QString text);
    void playNetAudio(QString url);

public:
    VoicePlatform voicePlatform = VoiceLocal;
#if defined(ENABLE_TEXTTOSPEECH)
    QTextToSpeech *tts = nullptr;
#endif
    XfyTTS* xfyTTS = nullptr;
    int voicePitch = 50;
    int voiceSpeed = 50;
    int voiceVolume = 50;
    QString voiceName;
    QStringList ttsQueue;
    QMediaPlayer* ttsPlayer = nullptr;
    bool ttsDownloading = false;
    MicrosoftTTS* msTTS = nullptr;
    QString msTTSFormat;
};

#endif // VOICESERVICE_H
