#ifndef ETERNALBLOCKDIALOG_H
#define ETERNALBLOCKDIALOG_H

#include <QDialog>
#include "externalblockuser.h"

namespace Ui {
class EternalBlockDialog;
}

class EternalBlockDialog : public QDialog
{
    Q_OBJECT

public:
    EternalBlockDialog(QList<EternalBlockUser>* users, QWidget *parent = nullptr);
    ~EternalBlockDialog();

private slots:
    void on_listWidget_activated(const QModelIndex &index);

    void on_listWidget_customContextMenuRequested(const QPoint &);

signals:
    void signalCancelEternalBlock(QString uid, qint64 roomId); // 仅取消永久禁言
    void signalCancelBlock(QString uid, qint64 roomId); // 取消永久禁言并取消禁言

private:
    Ui::EternalBlockDialog *ui;
    QList<EternalBlockUser>* users;
};

#endif // ETERNALBLOCKDIALOG_H
