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
    QString uid;
    QString uname;
    qint64 blockTime; // 上次禁言的时间

    static EternalBlockUser fromJson(QJsonObject json)
    {
        EternalBlockUser user;
        user.uid = json.value("uid").toString();
        user.uname = json.value("uname").toString();
        user.blockTime = static_cast<qint64>(json.value("time").toDouble());
        return user;
    }

    QJsonObject toJson() const
    {
        QJsonObject json;
        json.insert("uid", uid);
        json.insert("uname", uname);
        json.insert("time", blockTime);
        return json;
    }
};

class EternalBlockDialog : public QDialog
{
    Q_OBJECT

public:
    explicit EternalBlockDialog(QWidget *parent = nullptr);
    ~EternalBlockDialog();

private:
    Ui::EternalBlockDialog *ui;
};

#endif // ETERNALBLOCKDIALOG_H
