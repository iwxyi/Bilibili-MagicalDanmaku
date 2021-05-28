#ifndef MYSETTINGS_H
#define MYSETTINGS_H

#include <QSettings>
#include <QColor>

class MySettings : public QSettings
{
public:

    MySettings(QSettings::Scope scope, QObject *parent = nullptr)
        : QSettings(scope, parent)
    {}

    MySettings(QObject *parent = nullptr)
        : QSettings(parent)
    {}

    MySettings(const QString &fileName, QSettings::Format format, QObject *parent = nullptr)
        : QSettings(fileName, format, parent)
    {}

    MySettings(QSettings::Format format, QSettings::Scope scope, const QString &organization, const QString &application = QString(), QObject *parent = nullptr)
        : QSettings(format, scope, organization, application, parent)
    {}

    MySettings(QSettings::Scope scope, const QString &organization, const QString &application = QString(), QObject *parent = nullptr)
        : QSettings(scope, organization, application, parent)
    {}

    MySettings(const QString &organization, const QString &application = QString(), QObject *parent = nullptr)
        : QSettings(organization, application, parent)
    {}

    void set(QString key, QVariant var)
    {
        QSettings::setValue(key, var);
    }

    void set(QString key, QList<qint64> list)
    {
        QStringList sl;
        foreach (auto i, list)
            sl.append(QString::number(i));
        set(key, sl);
    }

    void set(QString key, QHash<qint64, int> hash)
    {
        QStringList sl;
        for (auto it = hash.begin(); it != hash.end(); it++)
            sl.append(QString::number(it.key())+ ":" + QString::number(it.value()));
        set(key, sl);
    }

    bool b(QString key, QVariant def = QVariant())
    {
        return QSettings::value(key, def).toBool();
    }

    QColor c(QString key, QVariant def = QVariant())
    {
        return QSettings::value(key, def).toString();
    }

    double d(QString key, QVariant def = QVariant())
    {
        return QSettings::value(key, def).toDouble();
    }

    int i(QString key, QVariant def = QVariant())
    {
        return QSettings::value(key, def).toInt();
    }

    qint64 l(QString key, QVariant def = QVariant())
    {
        return QSettings::value(key, def).toLongLong();
    }

    QString s(QString key, QVariant def = QVariant())
    {
        return QSettings::value(key, def).toString();
    }

    void assign(bool& val, QString key)
    {
        val = value(key, val).toBool();
    }

    void assign(double& val, QString key)
    {
        val = value(key, val).toDouble();
    }

    void assign(int& val, QString key)
    {
        val = value(key, val).toInt();
    }

    void assign(qint64& val, QString key)
    {
        val = value(key, val).toLongLong();
    }

    void assign(QString& val, QString key)
    {
        val = value(key, val).toString();
    }

    void assign(QColor& val, QString key)
    {
        val = value(key, val).toString();
    }

    void assign(QList<qint64>& val, QString key)
    {
        QStringList sl = value(key).toStringList();
        foreach (auto s, sl)
            val.append(s.toLongLong());
    }

    void assign(QHash<qint64, int>& val, QString key)
    {
        QStringList sl = value(key).toStringList();
        foreach (auto s, sl)
        {
            QStringList l = s.split(":");
            if (l.size() != 2)
                continue;
            val.insert(l.at(0).toLongLong(), l.at(1).toInt());
        }
    }
};

#endif // MYSETTINGS_H
