#include "mfaudioendpointcontrol_fixed.h"

#include <Functiondiscoverykeys_devpkey.h>
#include <VersionHelpers.h>
#include <cstdint>

#include <QDebug>

MFAudioEndpointControl_Fixed::MFAudioEndpointControl_Fixed(QObject *parent)
    : QAudioOutputSelectorControl(parent)
    , m_currentActivate(0)
{
    //
}

MFAudioEndpointControl_Fixed::~MFAudioEndpointControl_Fixed()
{
    clear_endpoints();
    clear_active();
}

void MFAudioEndpointControl_Fixed::clear_active()
{
    if (m_currentActivate != nullptr)
    {
        m_currentActivate->Release();
        m_currentActivate = nullptr;
    }

    m_activeEndpoint.clear();

    auto hlp = this->findChild<MFAudioEndpointControl_Fixed_Helper *>("MFAECF_HLP", Qt::FindDirectChildrenOnly);
    if (hlp != nullptr) {hlp->m_activeEndpoint.clear();} // Fix due to the way I get VTable address
}

void MFAudioEndpointControl_Fixed::clear_endpoints()
{
    foreach (LPWSTR wstrID, m_devices)
         CoTaskMemFree(wstrID);

    m_devices.clear();
}

QList<QString> MFAudioEndpointControl_Fixed::availableOutputs() const
{
    return m_devices.keys();
}

QString MFAudioEndpointControl_Fixed::outputDescription(const QString &name) const
{
    return name.section(QLatin1Char('\\'), -1);
}

QString MFAudioEndpointControl_Fixed::defaultOutput() const
{
    return m_defaultEndpoint;
}

QString MFAudioEndpointControl_Fixed::activeOutput() const
{
    return this->findChild<MFAudioEndpointControl_Fixed_Helper *>("MFAECF_HLP", Qt::FindDirectChildrenOnly)->m_activeEndpoint;
}

void MFAudioEndpointControl_Fixed::setActiveOutput(const QString &name)
{
    auto qq = this->findChild<MFAudioEndpointControl_Fixed_Helper *>("MFAECF_HLP", Qt::FindDirectChildrenOnly);
    QString &activeEndpointReal = qq->m_activeEndpoint;

    if (this->m_activeEndpoint == activeEndpointReal)
    {
        if (m_activeEndpoint == name)
            return;

        updateEndpoints();

        QMap<QString, LPWSTR>::iterator it = m_devices.find(name);
        if (it == m_devices.end())
            return;

        clear_active();

        LPWSTR wstrID = *it;
        IMFActivate *activate = NULL;
        HRESULT hr = MFCreateAudioRendererActivate(&activate);
        if (FAILED(hr)) {
            qWarning() << "Failed to create audio renderer activate";
            return;
        }

        if (wstrID) {
            hr = activate->SetString(MF_AUDIO_RENDERER_ATTRIBUTE_ENDPOINT_ID, wstrID);
        } else {
            //This is the default one that has been inserted in updateEndpoints(),
            //so give the activate a hint that we want to use the device for multimedia playback
            //then the media foundation will choose an appropriate one.

            //from MSDN:
            //The ERole enumeration defines constants that indicate the role that the system has assigned to an audio endpoint device.
            //eMultimedia: Music, movies, narration, and live music recording.
            hr = activate->SetUINT32(MF_AUDIO_RENDERER_ATTRIBUTE_ENDPOINT_ROLE, eMultimedia);
        }

        if (FAILED(hr)) {
            qWarning() << "Failed to set attribute for audio device" << name;
            activate->Release();
            return;
        }

        m_currentActivate = activate;
        m_activeEndpoint = name;

        activeEndpointReal = name;
    }
    else
    {
        QString ae_tmp = activeEndpointReal;
        activeEndpointReal = "";
        this->setActiveOutput(ae_tmp);
    }
}

IMFActivate*  MFAudioEndpointControl_Fixed::createActivate()
{
    setActiveOutput(m_activeEndpoint);

    return m_currentActivate;
}

QMap<QString, QString> MFAudioEndpointControl_Fixed::availableOutputsFriendly()
{
    QMap<QString, QString> names;
    names.insert("Default", "Default");

    IMMDeviceEnumerator *pEnum = NULL;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator),
                          NULL, CLSCTX_ALL,
                          __uuidof(IMMDeviceEnumerator),
                         (void**)&pEnum);
    if (SUCCEEDED(hr)) {
        IMMDeviceCollection *pDevices = NULL;
        hr = pEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pDevices);
        if (SUCCEEDED(hr)) {
            UINT count;
            hr = pDevices->GetCount(&count);
            if (SUCCEEDED(hr)) {
                for (UINT i = 0; i < count; ++i) {
                    IMMDevice *pDevice = NULL;
                    hr = pDevices->Item(i, &pDevice);
                    if (SUCCEEDED(hr)) {
                        LPWSTR wstrID = NULL;
                        hr = pDevice->GetId(&wstrID);
                        if (SUCCEEDED(hr)) {
                            IPropertyStore *pPropStore = NULL;
                            hr = pDevice->OpenPropertyStore(STGM_READ, &pPropStore);
                            if (SUCCEEDED(hr)) { // I hate this bracket style >.<
                                PROPVARIANT varName;
                                PropVariantInit(&varName);
                                hr = pPropStore->GetValue(PKEY_Device_FriendlyName, &varName);
                                if (SUCCEEDED(hr)) {
                                    names.insert(QString::fromWCharArray(varName.pwszVal),
                                                 QString::fromWCharArray(wstrID));
                                }
                                PropVariantClear(&varName);
                                pPropStore->Release();
                            }
                            CoTaskMemFree(wstrID);
                        }
                        pDevice->Release();
                    }
                }
            }
            pDevices->Release();
        }
        pEnum->Release();
    }

    return names;
}

void MFAudioEndpointControl_Fixed::updateEndpoints()
{
    clear_endpoints();

    m_defaultEndpoint = QString::fromLatin1("Default");
    m_devices.insert(m_defaultEndpoint, NULL);

    IMMDeviceEnumerator *pEnum = NULL;
    HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator),
                          NULL, CLSCTX_ALL,
                          __uuidof(IMMDeviceEnumerator),
                         (void**)&pEnum);
    if (SUCCEEDED(hr)) {
        IMMDeviceCollection *pDevices = NULL;
        hr = pEnum->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &pDevices);
        if (SUCCEEDED(hr)) {
            UINT count;
            hr = pDevices->GetCount(&count);
            if (SUCCEEDED(hr)) {
                for (UINT i = 0; i < count; ++i) {
                    IMMDevice *pDevice = NULL;
                    hr = pDevices->Item(i, &pDevice);
                    if (SUCCEEDED(hr)) {
                        LPWSTR wstrID = NULL;
                        hr = pDevice->GetId(&wstrID);
                        if (SUCCEEDED(hr)) {
                            QString deviceId = QString::fromWCharArray(wstrID);
                            m_devices.insert(deviceId, wstrID);
                        }
                        pDevice->Release();
                    }
                }
            }
            pDevices->Release();
        }
        pEnum->Release();
    }
}



MFAudioEndpointControl_Fixed_Helper::MFAudioEndpointControl_Fixed_Helper(QAudioOutputSelectorControl *parent)
: QObject(parent)
{
    if (parent != nullptr)
    {
        if (IsWindowsVistaOrGreater()) // Vista introduced Media Foundation
        {
            std::size_t *vfptr_old = reinterpret_cast<std::size_t *>(parent);
            *vfptr_old = *reinterpret_cast<std::size_t *>(&MFAudioEndpointControl_Fixed()); // Local VTable hooked

            this->setObjectName("MFAECF_HLP");
        }
    } else
    {
        qWarning() << ("MFAudioEndpointControl_Fixed_Helper: No parent.");
    }
}
