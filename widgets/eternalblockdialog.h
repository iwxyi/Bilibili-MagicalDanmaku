#ifndef ETERNALBLOCKDIALOG_H
#define ETERNALBLOCKDIALOG_H

#include <QDialog>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonDocument>
#include <QJsonParseError>

namespace Ui {
class EternalBlockDialog;
}

struct EternalBlockUser
{
    qint64 uid = 0;
    QString uname;
    qint64 time = 0; // 上次禁言的时间

    EternalBlockUser()
    {}

    EternalBlockUser(qint64 uid) : uid(uid)
    {}

    EternalBlockUser(qint64 uid, QString name, qint64 time)
        : uid(uid), uname(name), time(time)
    {}

    static EternalBlockUser fromJson(QJsonObject json)
    {
        EternalBlockUser user;
        user.uid = static_cast<qint64>(json.value("uid").toDouble());
        user.uname = json.value("uname").toString();
        user.time = static_cast<qint64>(json.value("time").toDouble());
        return user;
    }

    QJsonObject toJson() const
    {
        QJsonObject json;
        json.insert("uid", uid);
        json.insert("uname", uname);
        json.insert("time", time);
        return json;
    }

    bool operator==(const EternalBlockUser& another) const
    {
        return this->uid == another.uid;
    }
};

class EternalBlockDialog : public QDialog
{
    Q_OBJECT

public:
    EternalBlockDialog(QList<EternalBlockUser>* users, QWidget *parent = nullptr);
    ~EternalBlockDialog();

private slots:
    void on_listWidget_activated(const QModelIndex &index);

    void on_listWidget_customContextMenuRequested(const QPoint &pos);

signals:
    void signalCancelEternalBlock(qint64 uid);
    void signalCancelBlock(qint64 uid); // 取消永久禁言并取消禁言

private:
    Ui::EternalBlockDialog *ui;
    QList<EternalBlockUser>* users;
};

#endif // ETERNALBLOCKDIALOG_H
