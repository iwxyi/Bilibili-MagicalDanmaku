#ifndef MYJSON_H
#define MYJSON_H

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include "functional"

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

    MyJson(const QJsonObject& json) : QJsonObject(json)
    {
    }

    MyJson(const QByteArray& ba) : QJsonObject()
    {
        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(ba, &error);
        if (error.error == QJsonParseError::NoError)
        {
            if (doc.isObject())
            {
                *this = doc.object();
            }
            else
            {
                _err = "Not Json Object";
            }
        }
        else
        {
            _err = error.errorString();
        }
    }

    static MyJson from(const QByteArray& ba, bool* ok = nullptr, QString* errorString = nullptr)
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
            MyJson json;
            json._err = error.errorString();
            return json;
        }
    }

    QJsonArray a(const QString& key) const
    {
        return QJsonObject::value(key).toArray();
    }

    bool b(const QString& key, const bool defaultValue = false) const
    {
        return QJsonObject::value(key).toBool(defaultValue);
    }

    double d(const QString& key, const double defaultValue = 0) const
    {
        return QJsonObject::value(key).toDouble(defaultValue);
    }

    QString d2(const QString& key) const
    {
        return QString::number(d(key), 'f', 2);
    }

    int i(const QString& key, const int defaultValue = 0) const
    {
        return QJsonObject::value(key).toInt(defaultValue);
    }

    qint64 l(const QString& key, const qint64 defaultValue = 0) const
    {
        return qint64(QJsonObject::value(key).toDouble(defaultValue));
    }

    MyJson o(const QString& key) const
    {
        return MyJson(QJsonObject::value(key).toObject());
    }

    QString s(const QString& key, const QString& defaultValue = "") const
    {
        return QJsonObject::value(key).toString(defaultValue);
    }

    QByteArray toBa(QJsonDocument::JsonFormat format = QJsonDocument::Indented) const
    {
        return QJsonDocument(*this).toJson(format);
    }

    // 链式设置方法
    MyJson& set(const QString& key, const QJsonValue& value)
    {
        insert(key, value);
        return *this;
    }

    void eachVal(const QString& key, std::function<void(QJsonValue)> const valFunc) const
    {
        foreach (QJsonValue value, a(key))
        {
            valFunc(value);
        }
    }

    void each(const QString& key, std::function<void(QJsonObject)> const valFunc) const
    {
        foreach (QJsonValue value, a(key))
        {
            valFunc(value.toObject());
        }
    }

    // 带索引的遍历
    void eachWithIndex(const QString& key, std::function<void(int, QJsonObject)> const valFunc) const
    {
        QJsonArray arr = a(key);
        for (int i = 0; i < arr.size(); ++i)
        {
            valFunc(i, arr[i].toObject());
        }
    }

    // 过滤数组
    QJsonArray filter(const QString& key, std::function<bool(QJsonObject)> const filterFunc) const
    {
        QJsonArray result;
        each(key, [&](QJsonObject obj) {
            if (filterFunc(obj))
                result.append(obj);
        });
        return result;
    }

    // 映射数组
    QJsonArray map(const QString& key, std::function<QJsonValue(QJsonObject)> const mapFunc) const
    {
        QJsonArray result;
        each(key, [&](QJsonObject obj) {
            result.append(mapFunc(obj));
        });
        return result;
    }

    // 合并对象
    MyJson& merge(const MyJson& other)
    {
        for (auto it = other.begin(); it != other.end(); ++it)
        {
            insert(it.key(), it.value());
        }
        return *this;
    }

    // 深度合并
    MyJson& deepMerge(const MyJson& other)
    {
        for (auto it = other.begin(); it != other.end(); ++it)
        {
            if (contains(it.key()) && value(it.key()).isObject() && it.value().isObject())
            {
                MyJson currentObj = o(it.key());
                currentObj.deepMerge(MyJson(it.value().toObject()));
                insert(it.key(), currentObj);
            }
            else
            {
                insert(it.key(), it.value());
            }
        }
        return *this;
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
    QStringList ss(const QString& key) const
    {
        QStringList sl;
        if (!contains(key))
            return sl;
        eachVal(key, [&](QJsonValue val){
            sl.append(val.toString());
        });
        return sl;
    }

    // 验证JSON结构
    bool validate(const QStringList& requiredKeys) const
    {
        for (const QString& key : requiredKeys)
        {
            if (!contains(key))
            {
                _err = QString("Missing required key: %1").arg(key);
                return false;
            }
        }
        return true;
    }

    // 获取嵌套值（支持 "a.b.c" 格式）
    QJsonValue getNested(const QString& path, const QJsonValue& defaultValue = QJsonValue()) const
    {
        QStringList keys = path.split('.');
        MyJson current = *this;
        
        for (int i = 0; i < keys.size() - 1; ++i)
        {
            if (!current.contains(keys[i]) || !current.value(keys[i]).isObject())
                return defaultValue;
            current = current.value(keys[i]).toObject();
        }
        
        if (!current.contains(keys.last()))
            return defaultValue;
            
        return current.value(keys.last());
    }

    // 设置嵌套值
    MyJson& setNested(const QString& path, const QJsonValue& value)
    {
        QStringList keys = path.split('.');
        MyJson current = *this;
        
        for (int i = 0; i < keys.size() - 1; ++i)
        {
            if (!current.contains(keys[i]) || !current.value(keys[i]).isObject())
            {
                current.insert(keys[i], MyJson());
            }
            current = current.value(keys[i]).toObject();
        }
        
        current.insert(keys.last(), value);
        return *this;
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

    bool hasError() const
    {
        return !_err.isEmpty();
    }

    QString getError() const
    {
        return _err;
    }

private:
    mutable QString _err;
};

#endif // MYJSON_H
