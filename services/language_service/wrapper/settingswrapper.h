#ifndef SETTINGSWRAPPER_H
#define SETTINGSWRAPPER_H

#include <QObject>
#include <QSettings>
#include <QString>
#include <QVariant>
#include <QDebug>

class SettingsWrapper : public QObject
{
    Q_OBJECT
public:
    explicit SettingsWrapper(QSettings *settings, QObject *parent = nullptr)
        : QObject(parent), m_settings(settings)
    {
        if (!m_settings)
        {
            qWarning() << "Warning: SettingsWrapper initialized with a null QSettings pointer.";
        }
    }

    Q_INVOKABLE QVariant read(const QString &key, const QVariant &defaultValue = QVariant()) const
    {
        if (m_settings)
        {
            return m_settings->value(key, defaultValue);
        }
        return defaultValue;
    }

    Q_INVOKABLE void write(const QString &key, const QVariant &value)
    {
        if (m_settings)
        {
            m_settings->setValue(key, value);
            m_settings->sync(); // 确保立即写入
        }
        else
        {
            qWarning() << "Warning: Attempted to write to settings with a null QSettings pointer.";
        }
    }

    Q_INVOKABLE bool contains(const QString &key) const
    {
        if (m_settings)
        {
            return m_settings->contains(key);
        }
        return false;
    }

    Q_INVOKABLE void remove(const QString &key)
    {
        if (m_settings)
        {
            m_settings->remove(key);
            m_settings->sync();
        }
        else
        {
            qWarning() << "Warning: Attempted to remove from settings with a null QSettings pointer.";
        }
    }

private:
    QSettings *m_settings;
};

#endif // SETTINGSWRAPPER_H
