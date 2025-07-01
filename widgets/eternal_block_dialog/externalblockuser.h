#ifndef EXTERNALBLOCKUSER_H
#define EXTERNALBLOCKUSER_H

#include <QJsonObject>
#include <QJsonValue>
#include <QJsonDocument>
#include <QJsonParseError>

struct EternalBlockUser
{
    QString uid;
    qint64 roomId = 0;
    QString uname;
    QString upName;
    QString roomTitle;
    qint64 time = 0; // 上次禁言的时间
    QString msg;

    EternalBlockUser()
    {}

    EternalBlockUser(QString uid, qint64 roomId, QString msg) : uid(uid), roomId(roomId), msg(msg)
    {}

    EternalBlockUser(QString uid, qint64 roomId, QString name, QString upName, QString title, qint64 time, QString msg)
        : uid(uid), roomId(roomId), uname(name), upName(upName), roomTitle(title), time(time), msg(msg)
    {}

    static EternalBlockUser fromJson(QJsonObject json)
    {
        EternalBlockUser user;
        if (json.value("uid").isString())
            user.uid = json.value("uid").toString();
        else
            user.uid = QString::number(static_cast<qint64>(json.value("uid").toDouble()));
        user.roomId = qint64(json.value("roomId").toDouble());
        user.uname = json.value("uname").toString();
        user.upName = json.value("upName").toString();
        user.roomTitle = json.value("roomTitle").toString();
        user.time = static_cast<qint64>(json.value("time").toDouble());
        user.msg = json.value("msg").toString();
        return user;
    }

    QJsonObject toJson() const
    {
        QJsonObject json;
        json.insert("uid", uid);
        json.insert("roomId", roomId);
        json.insert("uname", uname);
        json.insert("upName", upName);
        json.insert("roomTitle", roomTitle);
        json.insert("time", time);
        json.insert("msg", msg);
        return json;
    }

    bool operator==(const EternalBlockUser& another) const
    {
        return this->uid == another.uid && this->roomId == another.roomId;
    }
};

#endif // EXTERNALBLOCKUSER_H
