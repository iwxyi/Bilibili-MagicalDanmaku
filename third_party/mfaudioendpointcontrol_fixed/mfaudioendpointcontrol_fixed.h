#ifndef MFAUDIOENDPOINTCONTROL_FIXED_H
#define MFAUDIOENDPOINTCONTROL_FIXED_H

#include <mfapi.h>
#include <mfidl.h>
#include <mmdeviceapi.h>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "oleaut32.lib")
#pragma comment(lib, "Mf.lib")

#include <QAudioOutputSelectorControl>

class MFAudioEndpointControl_Fixed : public QAudioOutputSelectorControl
{
    Q_OBJECT
public:
    MFAudioEndpointControl_Fixed(QObject *parent = 0);
    ~MFAudioEndpointControl_Fixed() override;

    QList<QString> availableOutputs() const override;

    QString outputDescription(const QString &name) const override;

    QString defaultOutput() const override;
    QString activeOutput() const override;

    void setActiveOutput(const QString &name) override;

    IMFActivate* createActivate();

    // Custom
    static QMap<QString, QString> availableOutputsFriendly();

private:
    void clear_active();
    void clear_endpoints();
    void updateEndpoints();

    QString m_defaultEndpoint;
    QString m_activeEndpoint;
    QMap<QString, LPWSTR> m_devices;
    IMFActivate *m_currentActivate;
};

class MFAudioEndpointControl_Fixed_Helper : public QObject
{
    Q_OBJECT
public:
    MFAudioEndpointControl_Fixed_Helper(QAudioOutputSelectorControl *parent);
    ~MFAudioEndpointControl_Fixed_Helper() override {}

    QString m_activeEndpoint;
};

#endif // MFAUDIOENDPOINTCONTROL_FIXED_H
