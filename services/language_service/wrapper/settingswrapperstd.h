#ifndef SETTINGSWRAPPERSTD_H
#define SETTINGSWRAPPERSTD_H

#include <QObject>
#include <QSettings>
#include <QString>
#include <QVariant>
#include <QDebug>
#include <QJsonObject>
class SettingsWrapperStd : public QObject
{
    Q_OBJECT
public:
    explicit SettingsWrapperStd(QSettings *settings, QObject *parent = nullptr)
        : QObject(parent), m_settings(settings)
    {
        if (!m_settings)
        {
            qWarning() << "Warning: SettingsWrapperStd initialized with a null QSettings pointer.";
        }
    }

    void setDefaultPrefix(const QString &prefix)
    {
        m_defaultPrefix = prefix;
    }

    Q_INVOKABLE QVariant read(const std::string &key) const
    {
        if (m_settings)
        {
            QString useKey = QString::fromStdString(key);
            if (!m_defaultPrefix.isEmpty() && !useKey.contains("/"))
            {
                useKey = m_defaultPrefix + "/" + useKey;
            }
            return m_settings->value(useKey);
        }
        return QVariant();
    }

    Q_INVOKABLE std::string readString(const std::string &key) const
    {
        return read(key).toString().toStdString();
    }

    Q_INVOKABLE int readInt(const std::string &key) const
    {
        return read(key).toInt();
    }

    Q_INVOKABLE double readDouble(const std::string &key) const
    {
        return read(key).toDouble();
    }

    Q_INVOKABLE bool readBool(const std::string &key) const
    {
        return read(key).toBool();
    }

    Q_INVOKABLE QList<QVariant> readList(const std::string &key) const
    {
        return read(key).toList();
    }

    Q_INVOKABLE QMap<QString, QVariant> readMap(const std::string &key) const
    {
        return read(key).toMap();
    }

    Q_INVOKABLE QHash<QString, QVariant> readHash(const std::string &key) const
    {
        return read(key).toHash();
    }

    Q_INVOKABLE QByteArray readByteArray(const std::string &key) const
    {
        return read(key).toByteArray();
    }

    Q_INVOKABLE QJsonObject readJson(const std::string &key) const
    {
        return read(key).toJsonObject();
    }

    Q_INVOKABLE void write(const std::string &key, const QVariant &value)
    {
        if (m_settings)
        {
            QString useKey = QString::fromStdString(key);
            if (!m_defaultPrefix.isEmpty() && !useKey.contains("/"))
            {
                useKey = m_defaultPrefix + "/" + useKey;
            }
            m_settings->setValue(useKey, value);
            m_settings->sync(); // 确保立即写入
        }
        else
        {
            qWarning() << "Warning: Attempted to write to settings with a null QSettings pointer.";
        }
    }

    Q_INVOKABLE void writeString(const std::string &key, const std::string &value)
    {
        write(key, QString::fromStdString(value));
    }

    Q_INVOKABLE void writeInt(const std::string &key, int value)
    {
        write(key, value);
    }

    Q_INVOKABLE void writeDouble(const std::string &key, double value)
    {
        write(key, value);
    }

    Q_INVOKABLE void writeBool(const std::string &key, bool value)
    {
        write(key, value);
    }

    Q_INVOKABLE void writeList(const std::string &key, const QList<QVariant> &value)
    {
        write(key, value);
    }

    Q_INVOKABLE void writeMap(const std::string &key, const QMap<QString, QVariant> &value)
    {
        write(key, value);
    }

    Q_INVOKABLE void writeHash(const std::string &key, const QHash<QString, QVariant> &value)
    {
        write(key, value);
    }

    Q_INVOKABLE void writeByteArray(const std::string &key, const QByteArray &value)
    {
        write(key, value);
    }

    Q_INVOKABLE void writeJson(const std::string &key, const QJsonObject &value)
    {
        write(key, value);
    }

    Q_INVOKABLE bool contains(const std::string &key) const
    {
        if (m_settings)
        {
            return m_settings->contains(QString::fromStdString(key));
        }
        return false;
    }

    Q_INVOKABLE void remove(const std::string &key)
    {
        if (m_settings)
        {
            m_settings->remove(QString::fromStdString(key));
            m_settings->sync();
        }
        else
        {
            qWarning() << "Warning: Attempted to remove from settings with a null QSettings pointer.";
        }
    }

private:
    QSettings *m_settings;
    QString m_defaultPrefix;
};

#endif // SETTINGSWRAPPERSTD_H
