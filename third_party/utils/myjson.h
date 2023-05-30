#ifndef MYJSON_H
#define MYJSON_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <functional>

#define jsona(x, y) y = x.a(#y)
#define JA(x, y) QJsonArray y = x.a(#y)
#define jsonb(x, y) y = x.b(#y)
#define JB(x, y) bool y = x.b(#y)
#define jsond(x, y) y = x.d(#y)
#define JF(x, y) double y = x.d(#y)
#define jsoni(x, y) y = x.i(#y)
#define JI(x, y) int y = x.i(#y)
#define jsonl(x, y) y = x.l(#y)
#define JL(x, y) long long y = x.l(#y)
#define jsono(x, y) y = x.o(#y)
#define JO(x, y) MyJson y = x.o(#y)
#define jsons(x, y) y = x.s(#y)
#define JS(x, y) QString y = x.s(#y)
#define jsonv(x, y) y = x.v(#y)
#define JV(x, y) QVariant y = x.v(#y)

class MyJson : public QJsonObject
{
public:
    MyJson()
    {
    }

    MyJson(QJsonObject json) : QJsonObject(json)
    {
    }

    MyJson(QByteArray ba) : QJsonObject(QJsonDocument::fromJson(ba).object())
    {

    }

    static MyJson from(QByteArray ba, bool* ok = nullptr, QString* errorString = nullptr)
    {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(ba, &error);
        if (error.error == QJsonParseError::NoError)
        {
            if (!doc.isObject())
            {
                if (ok)
                    *ok = false;
                if (errorString)
                    *errorString = "Not Json Object";
                return MyJson();
            }
            if (ok)
                *ok = true;
            return doc.object();
        }
        else
        {
            if (ok)
                *ok = false;
            if (errorString)
                *errorString = error.errorString();
            return MyJson();
        }
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

    QString d2(QString key) const
    {
        return QString::number(d(key), 'f', 2);
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

    QByteArray toBa(QJsonDocument::JsonFormat format = QJsonDocument::Indented) const
    {
        return QJsonDocument(*this).toJson(format);
    }

    void eachVal(QString key, std::function<void(QJsonValue)> const valFunc) const
    {
        foreach (QJsonValue value, a(key))
        {
            valFunc(value);
        }
    }

    void each(QString key, std::function<void(QJsonObject)> const valFunc) const
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

    QString msg() const
    {
        if (contains("msg"))
            return s("msg");
        if (contains("message"))
            return s("message");
        return "";
    }

    QString err() const
    {
        if (contains("err"))
            return s("err");
        if (contains("error"))
            return s("error");
        if (contains("errorMsg"))
            return s("errorMsg");
        if (contains("errMsg"))
            return s("errMsg");
        if (contains("errorMessage"))
            return s("errorMessage");
        return "";
    }

    QString errOrMsg() const
    {
        QString e = err();
        if (!e.isEmpty())
            return e;
        return msg();
    }

    // ["aaa", "bbb", "ccc"]
    QStringList ss(QString key) const
    {
        QStringList sl;
        if (!contains(key))
            return sl;
        eachVal(key, [&](QJsonValue val){
            sl.append(val.toString());
        });
        return sl;
    }

    void insertStringList(const QString& key, const QStringList& vals)
    {
        QJsonArray ar;
        foreach (auto s, vals)
        {
            ar.append(s);
        }
        insert(key, ar);
    }
};

#endif // MYJSON_H
