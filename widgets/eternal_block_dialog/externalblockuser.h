#ifndef EXTERNALBLOCKUSER_H
#define EXTERNALBLOCKUSER_H

#include <QJsonObject>
#include <QJsonValue>
#include <QJsonDocument>
#include <QJsonParseError>

struct EternalBlockUser
{
    qint64 uid = 0;
    qint64 roomId = 0;
    QString uname;
    QString upName;
    QString roomTitle;
    qint64 time = 0; // 上次禁言的时间

    EternalBlockUser()
    {}

    EternalBlockUser(qint64 uid, qint64 roomId) : uid(uid), roomId(roomId)
    {}

    EternalBlockUser(qint64 uid, qint64 roomId, QString name, QString upName, QString title, qint64 time)
        : uid(uid), roomId(roomId), uname(name), upName(upName), roomTitle(title), time(time)
    {}

    static EternalBlockUser fromJson(QJsonObject json)
    {
        EternalBlockUser user;
        user.uid = qint64(json.value("uid").toDouble());
        user.roomId = qint64(json.value("roomId").toDouble());
        user.uname = json.value("uname").toString();
        user.upName = json.value("upName").toString();
        user.roomTitle = json.value("roomTitle").toString();
        user.time = static_cast<qint64>(json.value("time").toDouble());
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
        return json;
    }

    bool operator==(const EternalBlockUser& another) const
    {
        return this->uid == another.uid && this->roomId == another.roomId;
    }
};

#endif // EXTERNALBLOCKUSER_H
