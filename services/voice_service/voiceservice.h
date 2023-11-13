#ifndef VOICESERVICE_H
#define VOICESERVICE_H

#include <QObject>
#if defined(ENABLE_TEXTTOSPEECH)
#include <QtTextToSpeech/QTextToSpeech>
#endif
#include <QMediaPlayer>
#include "xfytts.h"
#include "microsofttts.h"
#include "netinterface.h"

#define DEFAULT_MS_TTS_SSML_FORMAT "<speak version=\"1.0\" xmlns=\"http://www.w3.org/2001/10/synthesis\"\n\
        xmlns:mstts=\"https://www.w3.org/2001/mstts\" xml:lang=\"zh-CN\">\n\
     <voice name=\"zh-CN-XiaoxiaoNeural\">\n\
        <mstts:express-as style=\"affectionate\" >\n\
            <prosody rate=\"0%\" pitch=\"0%\">\n\
                %text%\n\
            </prosody>\n\
        </mstts:express-as>\n\
     </voice>\n\
</speak>"

enum VoicePlatform
{
    VoiceLocal,
    VoiceXfy,
    VoiceCustom,
    VoiceMS,
};

class VoiceService : public QObject, public NetInterface
{
    Q_OBJECT
public:
    explicit VoiceService(QObject *parent = nullptr);
    void initTTS();

signals:
    void signalInit();
    void signalInitFinished();
    void signalError(QString title, QString msg);

public slots:
    void speakText(QString text);
    void speakTextQueueNext();
    void voiceDownloadAndSpeak(QString text);
    void playNetAudio(QString url);
    void showError(QString title, QString msg);

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
