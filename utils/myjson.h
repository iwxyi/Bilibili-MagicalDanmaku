#ifndef MYJSON_H
#define MYJSON_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include "functional"

#define jsona(x, y) y = x.a(#y)
#define jsonb(x, y) y = x.b(#y)
#define jsond(x, y) y = x.d(#y)
#define jsoni(x, y) y = x.i(#y)
#define jsonl(x, y) y = x.l(#y)
#define jsono(x, y) y = x.o(#y)
#define jsons(x, y) y = x.s(#y)
#define jsonv(x, y) y = x.v(#y)

class MyJson : public QJsonObject
{
public:
    MyJson(QJsonObject json) : QJsonObject(json)
    {
    }

    MyJson(QByteArray ba) : QJsonObject(QJsonDocument::fromJson(ba).object())
    {

    }

    QJsonArray a(QString key) const
    {
        return QJsonObject::value(key).toArray();
    }

    bool b(QString key) const
    {
        return QJsonObject::value(key).toBool();
    }

    double d(QString key) const
    {
        return QJsonObject::value(key).toDouble();
    }

    int i(QString key) const
    {
        return QJsonObject::value(key).toInt();
    }

    qint64 l(QString key) const
    {
        return qint64(QJsonObject::value(key).toDouble());
    }

    MyJson o(QString key) const
    {
        return MyJson(QJsonObject::value(key).toObject());
    }

    QString s(QString key) const
    {
        return QJsonObject::value(key).toString();
    }

    QByteArray toBa() const
    {
        return QJsonDocument(*this).toJson();
    }

    void each(QString key, std::function<void(QJsonValue)> const valFunc)
    {
        foreach (QJsonValue value, a(key))
        {
            valFunc(value);
        }
    }

    void each(QString key, std::function<void(QJsonObject)> const valFunc)
    {
        foreach (QJsonValue value, a(key))
        {
            valFunc(value.toObject());
        }
    }

    int code() const
    {
        return i("code");
    }

    MyJson data() const
    {
        return o("data");
    }
};

#endif // MYJSON_H
